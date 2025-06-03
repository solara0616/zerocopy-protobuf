# Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
# Copyright (c) 2021 - 2022 by Apex.AI Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

# Build singleprocess example
cmake_minimum_required(VERSION 3.16)
project(example_protobuf)

include(GNUInstallDirs)

find_package(iceoryx_platform REQUIRED)
find_package(iceoryx_posh CONFIG REQUIRED)
find_package(iceoryx_hoofs CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)

include(IceoryxPackageHelper)
include(IceoryxPlatform)
include(IceoryxPlatformSettings)

find_package(protobuf REQUIRED)

set(ZERO_COPY_PUBLISHER_LINK_LIBS
    iceoryx_posh::iceoryx_posh_roudi
    protobuf::libprotobuf)

set(ZERO_COPY_PUBLISHER_SRCS
    ./protobuf_zero_copy_publisher.cpp
    ./person.pb.cc)

set(ZERO_COPY_SUBSCRIBER_LINK_LIBS
    iceoryx_posh::iceoryx_posh_roudi
    protobuf::libprotobuf)

set(ZERO_COPY_SUBSCRIBER_SRCS
    ./protobuf_zero_copy_subscriber.cpp
    ./person.pb.cc)

iox_add_executable(
    TARGET      proto_zerocopy_publiser
    # Since we have one process where multiple threads are created we need to increase the stack size
    STACK_SIZE  3500000
    FILES       ${ZERO_COPY_PUBLISHER_SRCS}
    LIBS        ${ZERO_COPY_PUBLISHER_LINK_LIBS}
)

iox_add_executable(
    TARGET      proto_zerocopy_subscriber
    # Since we have one process where multiple threads are created we need to increase the stack size
    STACK_SIZE  3500000
    FILES       ${ZERO_COPY_SUBSCRIBER_SRCS}
    LIBS        ${ZERO_COPY_SUBSCRIBER_LINK_LIBS}
)
