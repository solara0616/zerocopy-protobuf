// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 - 2022 by Apex.AI Inc. All rights reserved.
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
#ifndef IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_RECEIVER_INL
#define IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_RECEIVER_INL

#include "iceoryx_posh/internal/popo/building_blocks/chunk_receiver.hpp"
#include "iceoryx_posh/internal/posh_error_reporting.hpp"

namespace iox
{
namespace popo
{
inline constexpr const char* asStringLiteral(const ChunkReceiveResult value) noexcept
{
    switch (value)
    {
    case ChunkReceiveResult::TOO_MANY_CHUNKS_HELD_IN_PARALLEL:
        return "ChunkReceiveResult::TOO_MANY_CHUNKS_HELD_IN_PARALLEL";
    case ChunkReceiveResult::NO_CHUNK_AVAILABLE:
        return "ChunkReceiveResult::NO_CHUNK_AVAILABLE";
    }

    return "[Undefined ChunkReceiveResult]";
}

inline std::ostream& operator<<(std::ostream& stream, ChunkReceiveResult value) noexcept
{
    stream << asStringLiteral(value);
    return stream;
}

inline log::LogStream& operator<<(log::LogStream& stream, ChunkReceiveResult value) noexcept
{
    stream << asStringLiteral(value);
    return stream;
}

template <typename ChunkReceiverDataType>
inline ChunkReceiver<ChunkReceiverDataType>::ChunkReceiver(not_null<MemberType_t* const> chunkReceiverDataPtr) noexcept
    : Base_t(static_cast<typename ChunkReceiverDataType::ChunkQueueData_t*>(chunkReceiverDataPtr))
{
}

template <typename ChunkReceiverDataType>
inline const typename ChunkReceiver<ChunkReceiverDataType>::MemberType_t*
ChunkReceiver<ChunkReceiverDataType>::getMembers() const noexcept
{
    return reinterpret_cast<const MemberType_t*>(Base_t::getMembers());
}

template <typename ChunkReceiverDataType>
inline typename ChunkReceiver<ChunkReceiverDataType>::MemberType_t*
ChunkReceiver<ChunkReceiverDataType>::getMembers() noexcept
{
    return reinterpret_cast<MemberType_t*>(Base_t::getMembers());
}

template <typename ChunkReceiverDataType>
inline expected<std::vector<mepoo::ChunkHeader*>, ChunkReceiveResult>
ChunkReceiver<ChunkReceiverDataType>::tryGetChunkHeaders() noexcept
{
    auto popRet = this->tryPop();

    if (popRet.has_value())
    {
        auto sharedMultiChunk = *popRet;

        if (sharedMultiChunk.getChunkHeaders().size() == 0)
        {
            return err(ChunkReceiveResult::NO_CHUNK_AVAILABLE);
        }

        // if the application holds too many chunks, don't provide more
        if (getMembers()->m_chunksInUse.insert(sharedMultiChunk))
        {
            return ok(sharedMultiChunk.getChunkHeaders());
        }
        else
        {
            // release the chunk
            sharedMultiChunk = nullptr;
            return err(ChunkReceiveResult::TOO_MANY_CHUNKS_HELD_IN_PARALLEL);
        }
    }

    return err(ChunkReceiveResult::NO_CHUNK_AVAILABLE);
}

template <typename ChunkReceiverDataType>
inline void ChunkReceiver<ChunkReceiverDataType>::release(std::vector<mepoo::ChunkHeader*>& chunkHeaders) noexcept
{
    mepoo::SharedMultiChunk chunk(nullptr);

    if (!getMembers()->m_chunksInUse.remove(chunkHeaders, chunk))
    {
        IOX_REPORT(PoshError::POPO__CHUNK_RECEIVER_INVALID_CHUNK_TO_RELEASE_FROM_USER, iox::er::RUNTIME_ERROR);
    }
}

template <typename ChunkReceiverDataType>
inline void ChunkReceiver<ChunkReceiverDataType>::releaseAll() noexcept
{
    getMembers()->m_chunksInUse.cleanup();
    this->clear();
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_RECEIVER_INL
