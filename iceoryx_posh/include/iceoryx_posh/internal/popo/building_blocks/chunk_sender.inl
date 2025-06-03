// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 - 2022 by Apex.AI Inc. All rights reserved.
// Copyright (c) 2022 by NXP. All rights reserved.
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
#ifndef IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_SENDER_INL
#define IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_SENDER_INL

#include "iceoryx_posh/internal/popo/building_blocks/chunk_sender.hpp"
#include "iceoryx_posh/internal/posh_error_reporting.hpp"

namespace iox
{

template <>
constexpr popo::AllocationError
from<mepoo::MemoryManager::Error, popo::AllocationError>(const mepoo::MemoryManager::Error error) noexcept
{
    switch (error)
    {
    case mepoo::MemoryManager::Error::NO_MEMPOOLS_AVAILABLE:
        return popo::AllocationError::NO_MEMPOOLS_AVAILABLE;
    case mepoo::MemoryManager::Error::NO_MEMPOOL_FOR_REQUESTED_CHUNK_SIZE:
        return popo::AllocationError::NO_MEMPOOLS_AVAILABLE;
    case mepoo::MemoryManager::Error::MEMPOOL_OUT_OF_CHUNKS:
        return popo::AllocationError::RUNNING_OUT_OF_CHUNKS;
    }
    return popo::AllocationError::UNDEFINED_ERROR;
}

namespace popo
{
inline constexpr const char* asStringLiteral(const AllocationError value) noexcept
{
    switch (value)
    {
    case AllocationError::UNDEFINED_ERROR:
        return "AllocationError::UNDEFINED_ERROR";
    case AllocationError::NO_MEMPOOLS_AVAILABLE:
        return "AllocationError::NO_MEMPOOLS_AVAILABLE";
    case AllocationError::RUNNING_OUT_OF_CHUNKS:
        return "AllocationError::RUNNING_OUT_OF_CHUNKS";
    case AllocationError::TOO_MANY_CHUNKS_ALLOCATED_IN_PARALLEL:
        return "AllocationError::TOO_MANY_CHUNKS_ALLOCATED_IN_PARALLEL";
    case AllocationError::INVALID_PARAMETER_FOR_USER_PAYLOAD_OR_USER_HEADER:
        return "AllocationError::INVALID_PARAMETER_FOR_USER_PAYLOAD_OR_USER_HEADER";
    case AllocationError::INVALID_PARAMETER_FOR_REQUEST_HEADER:
        return "AllocationError::INVALID_PARAMETER_FOR_REQUEST_HEADER";
    }

    return "[Undefined AllocationError]";
}

inline std::ostream& operator<<(std::ostream& stream, AllocationError value) noexcept
{
    stream << asStringLiteral(value);
    return stream;
}

inline log::LogStream& operator<<(log::LogStream& stream, AllocationError value) noexcept
{
    stream << asStringLiteral(value);
    return stream;
}

template <typename ChunkSenderDataType>
inline ChunkSender<ChunkSenderDataType>::ChunkSender(not_null<MemberType_t* const> chunkSenderDataPtr) noexcept
    : Base_t(static_cast<typename ChunkSenderDataType::ChunkDistributorData_t* const>(chunkSenderDataPtr))
{
}

template <typename ChunkSenderDataType>
inline const typename ChunkSender<ChunkSenderDataType>::MemberType_t*
ChunkSender<ChunkSenderDataType>::getMembers() const noexcept
{
    return reinterpret_cast<const MemberType_t*>(Base_t::getMembers());
}

template <typename ChunkSenderDataType>
inline typename ChunkSender<ChunkSenderDataType>::MemberType_t* ChunkSender<ChunkSenderDataType>::getMembers() noexcept
{
    return reinterpret_cast<MemberType_t*>(Base_t::getMembers());
}

template <typename ChunkSenderDataType>
inline expected<mepoo::ChunkHeader*, AllocationError>
ChunkSender<ChunkSenderDataType>::tryAllocate(const UniquePortId originId,
                                              const uint64_t userPayloadSize,
                                              const uint32_t userPayloadAlignment,
                                              const uint32_t userHeaderSize,
                                              const uint32_t userHeaderAlignment) noexcept
{
    // use the chunk stored in m_lastChunkUnmanaged if:
    //   - there is a valid chunk
    //   - there is no other owner
    //   - the new user-payload still fits in it
    const auto chunkSettingsResult =
        mepoo::ChunkSettings::create(userPayloadSize, userPayloadAlignment, userHeaderSize, userHeaderAlignment);
    if (chunkSettingsResult.has_error())
    {
        return err(AllocationError::INVALID_PARAMETER_FOR_USER_PAYLOAD_OR_USER_HEADER);
    }

    const auto& chunkSettings = chunkSettingsResult.value();
    const uint64_t requiredChunkSize = chunkSettings.requiredChunkSize();

    auto& lastMultiChunkUnmanaged = getMembers()->m_lastMultiChunkUnmanaged;
    mepoo::ChunkHeader* lastChunkChunkHeader =
        lastMultiChunkUnmanaged.isNotLogicalNullptrAndHasNoOtherOwners() ? lastMultiChunkUnmanaged.getChunkHeader() : nullptr;

    if (!m_chunkManagementManagement && lastChunkChunkHeader && (lastChunkChunkHeader->chunkSize() >= requiredChunkSize))
    {
        auto sharedChunk = lastMultiChunkUnmanaged.releaseFirstToSharedChunk();
        m_chunkManagementManagement = lastMultiChunkUnmanaged.getChunkManagementManagement();
        if (getMembers()->m_chunksInUse.insert(sharedChunk, m_chunkManagementManagement,false))
        {
            auto chunkSize = lastChunkChunkHeader->chunkSize();
            lastChunkChunkHeader->~ChunkHeader();
            new (lastChunkChunkHeader) mepoo::ChunkHeader(chunkSize, chunkSettings);
            lastChunkChunkHeader->setOriginId(originId);
            return ok(lastChunkChunkHeader);
        }
        else
        {
            return err(AllocationError::TOO_MANY_CHUNKS_ALLOCATED_IN_PARALLEL);
        }
    }
    else
    {
        // BEGIN of critical section, chunk will be lost if the process terminates in this section
        // get a new chunk
        auto getChunkResult = getMembers()->m_memoryMgr->getChunk(chunkSettings);

        if (getChunkResult.has_error())
        {
            /// @todo iox-#1012 use error<E2>::from(E1); once available
            return err(into<AllocationError>(getChunkResult.error()));
        }

        auto& chunk = getChunkResult.value();

        bool initialized{false};
        if (m_chunkManagementManagement == nullptr)
        {
            if ((m_chunkManagementManagement = tryAllocateChunkManagementManagement()) == nullptr)
            {
                return err(into<AllocationError>(getChunkResult.error()));
            }
        }
        else
        {
            initialized = true;
        }

        // if the application allocated too much chunks, return no more chunks
        if (getMembers()->m_chunksInUse.insert(chunk, m_chunkManagementManagement, initialized))
        {
            // END of critical section
            chunk.getChunkHeader()->setOriginId(originId);
            return ok(chunk.getChunkHeader());
        }
        else
        {
            // release the allocated chunk
            chunk = nullptr;
            return err(AllocationError::TOO_MANY_CHUNKS_ALLOCATED_IN_PARALLEL);
        }
    }
}

template <typename ChunkSenderDataType>
inline mepoo::ChunkManagementManagement*
ChunkSender<ChunkSenderDataType>::tryAllocateChunkManagementManagement() noexcept
{
    return getMembers()->m_memoryMgr->getMultiChunk();
}

template <typename ChunkSenderDataType>
inline void ChunkSender<ChunkSenderDataType>::release(std::vector<mepoo::ChunkHeader*>& chunkHeaders) noexcept
{
    mepoo::SharedMultiChunk chunk(nullptr);
    // d'tor of SharedChunk will release the memory, we do not have to touch the returned chunk
    if (!getMembers()->m_chunksInUse.remove(chunkHeaders, chunk))
    {
        IOX_REPORT(PoshError::POPO__CHUNK_SENDER_INVALID_CHUNK_TO_FREE_FROM_USER, iox::er::RUNTIME_ERROR);
    }
}

template <typename ChunkSenderDataType>
inline uint64_t ChunkSender<ChunkSenderDataType>::send(std::vector<mepoo::ChunkHeader*>& chunkHeaders) noexcept
{
    uint64_t numberOfReceiverTheChunkWasDelivered{0};
    mepoo::SharedMultiChunk sharedMultiChunk{nullptr};
    // BEGIN of critical section, chunk will be lost if the process terminates in this section
    if (getChunkReadyForSend(chunkHeaders, sharedMultiChunk))
    {
        numberOfReceiverTheChunkWasDelivered = this->deliverToAllStoredQueues(sharedMultiChunk);

        getMembers()->m_lastMultiChunkUnmanaged.releaseToSharedChunk();
        getMembers()->m_lastMultiChunkUnmanaged = sharedMultiChunk;
    }
    // END of critical section

    return numberOfReceiverTheChunkWasDelivered;
}

template <typename ChunkSenderDataType>
inline bool ChunkSender<ChunkSenderDataType>::sendToQueue(mepoo::ChunkHeader* const chunkHeader,
                                                          const UniqueId uniqueQueueId,
                                                          const uint32_t lastKnownQueueIndex) noexcept
{
    //mepoo::SharedMultiChunk chunk(nullptr);
    // BEGIN of critical section, chunk will be lost if the process terminates in this section
    /*if (getChunkReadyForSend(chunkHeader, chunk))
    {
        auto deliveryResult = this->deliverToQueue(uniqueQueueId, lastKnownQueueIndex, chunk);

        getMembers()->m_lastMultiChunkUnmanaged.releaseToSharedChunk();
        getMembers()->m_lastMultiChunkUnmanaged = chunk;

        return !deliveryResult.has_error();
    }*/
    // END of critical section

    uint64_t id = uniqueQueueId;
    IOX_LOG(Info, "ChunkHeader: " << reinterpret_cast<uint64_t>(const_cast<mepoo::ChunkHeader*>(chunkHeader)));
    IOX_LOG(Info, "uniqueQueueId: " << id);
    IOX_LOG(Info, "lastKnownQueueIndex: " << lastKnownQueueIndex);

    return false;
}

template <typename ChunkSenderDataType>
inline void ChunkSender<ChunkSenderDataType>::pushToHistory(mepoo::ChunkHeader* const chunkHeader) noexcept
{
    mepoo::SharedChunk chunk(nullptr);
    // BEGIN of critical section, chunk will be lost if the process terminates in this section
    if (getChunkReadyForSend(chunkHeader, chunk))
    {
        this->addToHistoryWithoutDelivery(chunk);

        getMembers()->m_lastChunkUnmanaged.releaseToSharedChunk();
        getMembers()->m_lastChunkUnmanaged = chunk;
    }
    // END of critical section
}

template <typename ChunkSenderDataType>
inline optional<const mepoo::ChunkHeader*> ChunkSender<ChunkSenderDataType>::tryGetPreviousChunk() const noexcept
{
    if (!getMembers()->m_lastMultiChunkUnmanaged.isLogicalNullptr())
    {
        return getMembers()->m_lastMultiChunkUnmanaged.getChunkHeader();
    }
    else
    {
        return nullopt_t();
    }
}

template <typename ChunkSenderDataType>
inline void ChunkSender<ChunkSenderDataType>::releaseAll() noexcept
{
    getMembers()->m_chunksInUse.cleanup();
    this->cleanup();
    getMembers()->m_lastMultiChunkUnmanaged.releaseToSharedChunk();
}

template <typename ChunkSenderDataType>
inline void ChunkSender<ChunkSenderDataType>::resetChunkManagementManagement() noexcept
{
    m_chunkManagementManagement = nullptr;
}

template <typename ChunkSenderDataType>
inline bool ChunkSender<ChunkSenderDataType>::getChunkReadyForSend(const mepoo::ChunkHeader* const chunkHeader,
                                                                   mepoo::SharedChunk& chunk) noexcept
{
    if (getMembers()->m_chunksInUse.remove(chunkHeader, chunk))
    {
        chunk.getChunkHeader()->setSequenceNumber(getMembers()->m_sequenceNumber++);
        return true;
    }
    else
    {
        IOX_REPORT(PoshError::POPO__CHUNK_SENDER_INVALID_CHUNK_TO_SEND_FROM_USER, iox::er::RUNTIME_ERROR);
        return false;
    }
}

template <typename ChunkSenderDataType>
inline bool ChunkSender<ChunkSenderDataType>::getChunkReadyForSend(std::vector<mepoo::ChunkHeader*>& chunkHeaders
                                                                   , mepoo::SharedMultiChunk& chunk) noexcept
{
    if (getMembers()->m_chunksInUse.remove(chunkHeaders, chunk))
    {
        chunk.getChunkHeader()->setSequenceNumber(getMembers()->m_sequenceNumber++);
        return true;
    }
    else
    {
        IOX_REPORT(PoshError::POPO__CHUNK_SENDER_INVALID_CHUNK_TO_SEND_FROM_USER, iox::er::RUNTIME_ERROR);
        return false;
    }
}

} // namespace popo
} // namespace iox

#endif // IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_SENDER_INL
