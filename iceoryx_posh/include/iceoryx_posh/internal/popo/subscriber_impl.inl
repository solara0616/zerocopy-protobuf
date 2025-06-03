// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
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

#ifndef IOX_POSH_POPO_TYPED_SUBSCRIBER_IMPL_INL
#define IOX_POSH_POPO_TYPED_SUBSCRIBER_IMPL_INL

#include "iceoryx_posh/internal/popo/subscriber_impl.hpp"

namespace iox
{
namespace popo
{
template <typename T, typename H, typename BaseSubscriberType>
inline SubscriberImpl<T, H, BaseSubscriberType>::SubscriberImpl(PortType&& port) noexcept
    : BaseSubscriberType(std::move(port))
{
}

template <typename T, typename H, typename BaseSubscriberType>
inline SubscriberImpl<T, H, BaseSubscriberType>::SubscriberImpl(const capro::ServiceDescription& service,
                                                                const SubscriberOptions& subscriberOptions) noexcept
    : BaseSubscriberType(service, subscriberOptions)
{
}

template <typename T, typename H, typename BaseSubscriberType>
inline expected<Sample<const T, const H>, ChunkReceiveResult> SubscriberImpl<T, H, BaseSubscriberType>::take() noexcept
{
    auto result = BaseSubscriberType::takeChunks();
    if (result.has_error())
    {
        return err(result.error());
    }

    auto& chunkHeaders = result.value();
    if (chunkHeaders.size() == 0)
    {
        return err(result.error());
    }

    mepoo::ChunkHeader* chunkHeader = chunkHeaders[0];
    auto userPayloadPtr = static_cast<const T*>(chunkHeader->userPayload());
    userPayloadPtr = reinterpret_cast<const T*>((reinterpret_cast<uint64_t>(userPayloadPtr) + 120));
    auto samplePtr = iox::unique_ptr<const T>(userPayloadPtr, [this, &chunkHeaders](const T* userPayload) {
        userPayload = reinterpret_cast<const T*>(reinterpret_cast<uint64_t>(userPayload) - 120);
        this->port().releaseChunk(chunkHeaders);
    });
    return ok<Sample<const T, const H>>(std::move(samplePtr));
}

template <typename T, typename H, typename BaseSubscriberType>
inline expected<Sample<const T, const H>, ChunkReceiveResult> SubscriberImpl<T, H, BaseSubscriberType>::takeMultiChunk() noexcept
{
    auto result = BaseSubscriberType::takeChunks();
    if (result.has_error())
    {
        return err(result.error());
    }

    std::vector<mepoo::ChunkHeader*>& chunkHeaders = result.value();
    mepoo::ChunkHeader* chunkHeader = chunkHeaders[0];
    auto userPayloadPtr = static_cast<const T*>(chunkHeader->userPayload());
    userPayloadPtr = reinterpret_cast<const T*>((reinterpret_cast<uint64_t>(userPayloadPtr) + 120));
    auto samplePtr = iox::unique_ptr<const T>(userPayloadPtr, [this, &chunkHeaders](const T* userPayload) {
        userPayload = reinterpret_cast<const T*>(reinterpret_cast<uint64_t>(userPayload) - 120);
        this->port().releaseChunk(chunkHeaders);
    });
    return ok<Sample<const T, const H>>(std::move(samplePtr));
}

template <typename T, typename H, typename BaseSubscriberType>
inline SubscriberImpl<T, H, BaseSubscriberType>::~SubscriberImpl() noexcept
{
    BaseSubscriberType::m_trigger.reset();
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_TYPED_SUBSCRIBER_IMPL_INL
