// Copyright (c) 2020 - 2021 by Robert Bosch GmbH All rights reserved.
// Copyright (c) 2020 - 2022 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "iceoryx_posh/iceoryx_posh_config.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/internal/roudi/roudi.hpp"
#include "iceoryx_posh/popo/publisher.hpp"
#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/popo/sample.hpp"
#include "iceoryx_posh/roudi/iceoryx_roudi_components.hpp"
#include "iceoryx_posh/runtime/posh_runtime_single_process.hpp"
#include "iox/detail/convert.hpp"
#include "iox/logging.hpp"
#include "iox/signal_watcher.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <mutex>
#include <thread>

#include <google/protobuf/arena.h>
#include <google/protobuf/message.h>

#include "person.pb.h"

struct WriteBlock {
    size_t size;

};

constexpr std::chrono::milliseconds CYCLE_TIME{100};

void consoleOutput(const char* source, const char* arrow, const uint64_t counter)
{
    static std::mutex consoleOutputMutex;

    std::lock_guard<std::mutex> lock(consoleOutputMutex);
    std::cout << source << arrow << counter << std::endl;
}

iox::popo::Publisher<tutorial::Person, WriteBlock>* get_publisher_singleton_instance() {
    static iox::popo::Publisher<tutorial::Person, WriteBlock>* pPub = nullptr;
    if (pPub == nullptr) {
        static iox::popo::PublisherOptions publisherOptions;
        publisherOptions.historyCapacity = 10U;
        static iox::popo::Publisher<tutorial::Person, WriteBlock> publisher({"NonZeroCopy", "Protobuf", "Demo"}, publisherOptions);
        pPub = &publisher;
    }
    return pPub;
}

void* block_alloc(size_t size, size_t& actualSize) {
    auto* pPub = get_publisher_singleton_instance();

    if (pPub != nullptr) {
        return pPub->loanBlock(size, actualSize);
    }
    return nullptr;
}

void block_dealloc(void* ptr, size_t size) {
    auto* pPub = get_publisher_singleton_instance();

    if (pPub != nullptr) {
        return pPub->releaseBlock(ptr);
    }
}

void publisher()
{
    google::protobuf::ArenaOptions options;
    options.block_alloc = block_alloc;
    options.block_dealloc = block_dealloc;

    int32_t counter = 0;

    iox::popo::Publisher<tutorial::Person, WriteBlock>* pPub = get_publisher_singleton_instance();

    //! [send]
    constexpr const char GREEN_RIGHT_ARROW[] = "\033[32m->\033[m ";
    while (!iox::hasTerminationRequested())
    {

        tutorial::Person person;
        person.set_id(counter++);
        person.add_value(counter);
        person.add_value(counter + 1);
        person.add_value(counter + 2);
        person.set_name("NonZeroCopy" + std::to_string(counter));

        WriteBlock wb;
        wb.size = person.ByteSizeLong();
        size_t actualSize;
        void* buf = block_alloc(wb.size, actualSize);
        person.SerializeToArray(buf, wb.size);
        pPub->getSample(buf).and_then([&](auto& sample) {
            //! [loan was successful]
            sample.getUserHeader().size = wb.size;
            consoleOutput("Sending   ", GREEN_RIGHT_ARROW, person.id());
            sample.publish();
        })
        .or_else([&](auto& error) {
            //! [loan failed]
            std::cout << " could not loan sample! Error code: " << error << std::endl;
            //! [loan failed]
        });

        std::this_thread::sleep_for(CYCLE_TIME);
    }
    //! [send]
}

int main()
{
    // set the log level to info to to have the output for launch_testing
    //! [log level]
    iox::log::Logger::init(iox::log::LogLevel::Info);
    //! [log level]

    //! [roudi config]
    iox::IceoryxConfig config = iox::IceoryxConfig().setDefaults();
    config.sharesAddressSpaceWithApplications = true;
    iox::roudi::IceOryxRouDiComponents roudiComponents(config);
    //! [roudi config]

    //! [roudi]
    iox::roudi::RouDi roudi(roudiComponents.rouDiMemoryManager, roudiComponents.portManager, config);
    //! [roudi]

    // create a single process runtime for inter thread communication
    //! [runtime]
    iox::runtime::PoshRuntimeSingleProcess runtime("iox-cpp-proto-non-zerocopy-publisher");
    //! [runtime]

    //! [run]
    std::thread publisherThread(publisher);//, subscriberThread(subscriber);

    iox::waitForTerminationRequest();

    publisherThread.join();
    //subscriberThread.join();

    std::cout << "Finished" << std::endl;
    //! [run]
}
