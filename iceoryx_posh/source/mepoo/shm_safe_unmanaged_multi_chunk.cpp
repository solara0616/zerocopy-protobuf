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

#include "iceoryx_posh/internal/mepoo/shm_safe_unmanaged_multi_chunk.hpp"
#include "iox/assertions.hpp"

namespace iox
{
namespace mepoo
{
// Torn writes are problematic since RouDi needs to cleanup all chunks when an application crashes. If the size is
// larger than 8 bytes on a 64 bit system, torn writes happens and the data is only partially written when the
// application crashes at the wrong time. RouDi would then read corrupt data and try to access invalid memory.
static_assert(sizeof(ShmSafeUnmanagedMultiChunk) <= 8U,
              "The ShmSafeUnmanagedChunk size must not exceed 64 bit to prevent torn writes!");
// This ensures that the address of the ShmSafeUnmanagedChunk object is appropriately aligned to be accessed within one
// CPU cycle, i.e. if the size is 8 and the alignment is 4 it could be placed at an address with modulo 4 which would
// also result in torn writes.
static_assert(sizeof(ShmSafeUnmanagedMultiChunk) == alignof(ShmSafeUnmanagedMultiChunk),
              "A ShmSafeUnmanagedChunk must be placed on an address which does not cross the native alignment!");
// This is important for the use in the SOFI where under some conditions the copy operation could work on partially
// obsolet data and therefore non-trivial copy ctor/assignment operator or dtor would work on corrupted data.
static_assert(std::is_trivially_copyable<ShmSafeUnmanagedMultiChunk>::value,
              "The ShmSafeUnmanagedChunk must be trivially copyable to prevent Frankenstein objects when the copy ctor "
              "works on half dead objects!");

ShmSafeUnmanagedMultiChunk::ShmSafeUnmanagedMultiChunk(ChunkManagementManagement* chunkManagementManagement) noexcept
{
    RelativePointer<ChunkManagementManagement> chunkMgntMgnt(chunkManagementManagement);
    auto id = chunkMgntMgnt.getId();
    auto offset = chunkMgntMgnt.getOffset();
    IOX_ENFORCE(id <= RelativePointerData::ID_RANGE, "RelativePointer id must fit into id type!");
    IOX_ENFORCE(offset <= RelativePointerData::OFFSET_RANGE, "RelativePointer offset must fit into offset type!");
    m_chunkManagementManagement = RelativePointerData(static_cast<RelativePointerData::identifier_t>(id), offset);
}

ShmSafeUnmanagedMultiChunk::ShmSafeUnmanagedMultiChunk(mepoo::SharedMultiChunk chunk) noexcept
{
    // this is only necessary if it's not an empty chunk
    if (chunk)
    {
        RelativePointer<mepoo::ChunkManagementManagement> ptr{chunk.release()};
        auto id = ptr.getId();
        auto offset = ptr.getOffset();
        IOX_ENFORCE(id <= RelativePointerData::ID_RANGE, "RelativePointer id must fit into id type!");
        IOX_ENFORCE(offset <= RelativePointerData::OFFSET_RANGE, "RelativePointer offset must fit into offset type!");
        /// @todo iox-#1196 Unify types to uint64_t
        m_chunkManagementManagement = RelativePointerData(static_cast<RelativePointerData::identifier_t>(id), offset);
    }
}

SharedMultiChunk ShmSafeUnmanagedMultiChunk::releaseToSharedChunk() noexcept
{
    if (m_chunkManagementManagement.isLogicalNullptr())
    {
        return SharedMultiChunk();
    }

    auto chunkMgmtMgmt =
        RelativePointer<mepoo::ChunkManagementManagement>(m_chunkManagementManagement.offset(), 
                                                         segment_id_t{m_chunkManagementManagement.id()});
    m_chunkManagementManagement.reset();
    return SharedMultiChunk(chunkMgmtMgmt.get());
}

SharedMultiChunk ShmSafeUnmanagedMultiChunk::cloneToSharedChunk() noexcept
{
    if (m_chunkManagementManagement.isLogicalNullptr())
    {
        return SharedMultiChunk();
    }
    auto chunkMgmtMgmt =
        RelativePointer<mepoo::ChunkManagementManagement>(m_chunkManagementManagement.offset(), 
                                                         segment_id_t{m_chunkManagementManagement.id()});
#if (defined(__GNUC__) && __GNUC__ == 13 && !defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
    chunkMgmtMgmt->m_referenceCounter.fetch_add(1U, std::memory_order_relaxed);
#if (defined(__GNUC__) && __GNUC__ == 13 && !defined(__clang__))
#pragma GCC diagnostic pop
#endif
    return SharedMultiChunk(chunkMgmtMgmt.get());
}

SharedChunk ShmSafeUnmanagedMultiChunk::cloneFirstToSharedChunk() noexcept
{
    if (m_chunkManagementManagement.isLogicalNullptr())
    {
        return SharedChunk();
    }

    auto chunkMgmtMgmt =
        RelativePointer<mepoo::ChunkManagementManagement>(m_chunkManagementManagement.offset(), 
                                                         segment_id_t{m_chunkManagementManagement.id()});
    if (chunkMgmtMgmt->m_chunkManagements.size() == 0)
    {
        return SharedChunk();
    }

#if (defined(__GNUC__) && __GNUC__ == 13 && !defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
    chunkMgmtMgmt->m_referenceCounter.fetch_add(1U, std::memory_order_relaxed);
#if (defined(__GNUC__) && __GNUC__ == 13 && !defined(__clang__))
#pragma GCC diagnostic pop
#endif

    auto begin = chunkMgmtMgmt->m_chunkManagements.begin();
    auto iter = ++begin;
    if (iter != chunkMgmtMgmt->m_chunkManagements.end())
    {
        SharedChunk chunk(iter->get());
        iter = chunkMgmtMgmt->m_chunkManagements.erase(iter);
    }

    return SharedChunk(begin->get());
}

SharedChunk ShmSafeUnmanagedMultiChunk::releaseFirstToSharedChunk() noexcept
{
    if (m_chunkManagementManagement.isLogicalNullptr())
    {
        return SharedChunk();
    }
    auto chunkMgmtMgmt =
        RelativePointer<mepoo::ChunkManagementManagement>(m_chunkManagementManagement.offset(), 
                                                         segment_id_t{m_chunkManagementManagement.id()});
    
    if (chunkMgmtMgmt->m_chunkManagements.size() == 0)
    {
        return SharedChunk();
    }

    auto begin = chunkMgmtMgmt->m_chunkManagements.begin();
    auto iter = ++begin;
    if (iter != chunkMgmtMgmt->m_chunkManagements.end())
    {
        SharedChunk chunk(iter->get());
        iter = chunkMgmtMgmt->m_chunkManagements.erase(iter);
    }

    return SharedChunk(begin->get());
}

bool ShmSafeUnmanagedMultiChunk::isLogicalNullptr() const noexcept
{
    return m_chunkManagementManagement.isLogicalNullptr();
}

ChunkHeader* ShmSafeUnmanagedMultiChunk::getChunkHeader() noexcept
{
    if (m_chunkManagementManagement.isLogicalNullptr())
    {
        return nullptr;
    }
    auto chunkMgmtMgmt =
        RelativePointer<mepoo::ChunkManagementManagement>(m_chunkManagementManagement.offset(), 
                                                         segment_id_t{m_chunkManagementManagement.id()});
    if (chunkMgmtMgmt->m_chunkManagements.size() == 0)
    {
        return nullptr;
    }

    auto begin = chunkMgmtMgmt->m_chunkManagements.begin();
    return (*begin)->m_chunkHeader.get();
}

ChunkManagementManagement* ShmSafeUnmanagedMultiChunk::getChunkManagementManagement() noexcept
{
    if (m_chunkManagementManagement.isLogicalNullptr())
    {
        return nullptr;
    }

    auto chunkMgmtMgmt =
        RelativePointer<mepoo::ChunkManagementManagement>(m_chunkManagementManagement.offset(), 
                                                         segment_id_t{m_chunkManagementManagement.id()});
    if (chunkMgmtMgmt->m_chunkManagements.size() == 0)
    {
        return nullptr;
    }

    return chunkMgmtMgmt.get();
}

std::vector<ChunkHeader*> ShmSafeUnmanagedMultiChunk::getChunkHeaders() noexcept
{
    if (m_chunkManagementManagement.isLogicalNullptr())
    {
        return {nullptr};
    }

    auto chunkMgmtMgmt =
        RelativePointer<mepoo::ChunkManagementManagement>(m_chunkManagementManagement.offset(), 
                                                         segment_id_t{m_chunkManagementManagement.id()});
    if (chunkMgmtMgmt->m_chunkManagements.size() == 0)
    {
        return {nullptr};
    }

    std::vector<ChunkHeader*> chunkHeaders;
    auto iter = chunkMgmtMgmt->m_chunkManagements.begin();
    while (iter != chunkMgmtMgmt->m_chunkManagements.end())
    {
        chunkHeaders.push_back((*iter)->m_chunkHeader.get());
        iter++;
    }
    
    return chunkHeaders;
}

const ChunkHeader* ShmSafeUnmanagedMultiChunk::getChunkHeader() const noexcept
{
    return const_cast<ShmSafeUnmanagedMultiChunk*>(this)->getChunkHeader();
}

bool ShmSafeUnmanagedMultiChunk::isNotLogicalNullptrAndHasNoOtherOwners() const noexcept
{
    if (m_chunkManagementManagement.isLogicalNullptr())
    {
        return false;
    }

    auto chunkMgmtMgmt =
        RelativePointer<mepoo::ChunkManagementManagement>(m_chunkManagementManagement.offset(), 
                                                         segment_id_t{m_chunkManagementManagement.id()});
    return chunkMgmtMgmt->m_referenceCounter.load(std::memory_order_relaxed) == 1U;
}

bool ShmSafeUnmanagedMultiChunk::addChunkManagement(const not_null<ChunkManagement*> chunkManagement) noexcept
{
    if (m_chunkManagementManagement.isLogicalNullptr())
    {
        return false;
    }

    auto chunkMgmtMgmt =
        RelativePointer<mepoo::ChunkManagementManagement>(m_chunkManagementManagement.offset(), 
                                                         segment_id_t{m_chunkManagementManagement.id()});
    return chunkMgmtMgmt->addChunkManagement(chunkManagement);
}

} // namespace mepoo
} // namespace iox
