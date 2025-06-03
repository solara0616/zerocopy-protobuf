// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 - 2022 by Apex.AI Inc. All rights reserved.
// Copyright (c) 2025 by Latitude AI. All rights reserved.
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

#include "iceoryx_posh/internal/mepoo/shared_multi_chunk.hpp"
#include "iceoryx_posh/internal/mepoo/memory_manager.hpp"

namespace iox
{
namespace mepoo
{
SharedMultiChunk::SharedMultiChunk(ChunkManagementManagement* const resource) noexcept
    : m_chunkManagementManagement(resource)
{
}

SharedMultiChunk::SharedMultiChunk(const SharedMultiChunk& rhs) noexcept
{
    *this = rhs;
}

SharedMultiChunk::SharedMultiChunk(SharedMultiChunk&& rhs) noexcept
{
    *this = std::move(rhs);
}

SharedMultiChunk::~SharedMultiChunk() noexcept
{
    decrementReferenceCounter();
}

void SharedMultiChunk::incrementReferenceCounter() noexcept
{
    if (m_chunkManagementManagement != nullptr)
    {
        m_chunkManagementManagement->m_referenceCounter.fetch_add(1U, std::memory_order_relaxed);
    }
}

void SharedMultiChunk::decrementReferenceCounter() noexcept
{
    if ((m_chunkManagementManagement != nullptr)
        && (m_chunkManagementManagement->m_referenceCounter.fetch_sub(1U, std::memory_order_relaxed) == 1U))
    {
        auto iter = m_chunkManagementManagement->m_chunkManagements.begin();
        while (iter != m_chunkManagementManagement->m_chunkManagements.end())
        {
            MemoryManager::freeChunk(*(iter->get()));
            iter++;
        }

        m_chunkManagementManagement->m_chunkManagementPool->freeChunk(m_chunkManagementManagement);
        m_chunkManagementManagement = nullptr;
    }
}

SharedMultiChunk& SharedMultiChunk::operator=(const SharedMultiChunk& rhs) noexcept
{
    if (this != &rhs)
    {
        decrementReferenceCounter();
        m_chunkManagementManagement = rhs.m_chunkManagementManagement;
        incrementReferenceCounter();
    }
    return *this;
}

SharedMultiChunk& SharedMultiChunk::operator=(SharedMultiChunk&& rhs) noexcept
{
    if (this != &rhs)
    {
        decrementReferenceCounter();
        m_chunkManagementManagement = std::move(rhs.m_chunkManagementManagement);
        rhs.m_chunkManagementManagement = nullptr;
    }
    return *this;
}

void* SharedMultiChunk::getUserPayload() const noexcept
{
    if (m_chunkManagementManagement == nullptr || 
        m_chunkManagementManagement->m_chunkManagements.size() == 0)
    {
        return nullptr;
    }
    else
    {
        RelativePointer<ChunkManagement>& ChunkManagement = *m_chunkManagementManagement->m_chunkManagements.begin();
        return ChunkManagement->m_chunkHeader->userPayload();
    }
}

bool SharedMultiChunk::operator==(const SharedMultiChunk& rhs) const noexcept
{
    return m_chunkManagementManagement == rhs.m_chunkManagementManagement;
}

bool SharedMultiChunk::operator==(const void* const rhs) const noexcept
{
    return getUserPayload() == rhs;
}

bool SharedMultiChunk::operator!=(const SharedMultiChunk& rhs) const noexcept
{
    return !(*this == rhs);
}

bool SharedMultiChunk::operator!=(const void* const rhs) const noexcept
{
    return !(*this == rhs);
}

SharedMultiChunk::operator bool() const noexcept
{
    return m_chunkManagementManagement != nullptr;
}

ChunkHeader* SharedMultiChunk::getChunkHeader() const noexcept
{
    if (m_chunkManagementManagement == nullptr ||
        m_chunkManagementManagement->m_chunkManagements.size() == 0)
    {
        return nullptr;
    }
    else
    {
        RelativePointer<ChunkManagement>& ChunkManagement = *m_chunkManagementManagement->m_chunkManagements.begin();
        return ChunkManagement->m_chunkHeader.get();
    }
}

std::vector<ChunkHeader*> SharedMultiChunk::getChunkHeaders() const noexcept
{
    if (m_chunkManagementManagement == nullptr ||
        m_chunkManagementManagement->m_chunkManagements.size() == 0)
    {
        return {nullptr};
    }

    std::vector<ChunkHeader*> chunkHeaders;
    auto iter = m_chunkManagementManagement->m_chunkManagements.begin();
    while (iter != m_chunkManagementManagement->m_chunkManagements.end())
    {
        chunkHeaders.push_back((*iter)->m_chunkHeader.get());
        iter++;
    }

    return chunkHeaders;
}

ChunkManagementManagement* SharedMultiChunk::release() noexcept
{
    ChunkManagementManagement* returnValue = m_chunkManagementManagement;
    m_chunkManagementManagement = nullptr;
    return returnValue;
}

ChunkManagement* SharedMultiChunk::getFirstChunkManagement() noexcept
{
    if (m_chunkManagementManagement == nullptr ||
        m_chunkManagementManagement->m_chunkManagements.size() == 0)
    {
        return nullptr;
    }

    return (m_chunkManagementManagement->m_chunkManagements.begin())->get();
}

bool SharedMultiChunk::addChunkManagement(const not_null<ChunkManagement*> chunkManagement) noexcept
{
    return m_chunkManagementManagement->addChunkManagement(chunkManagement);
}

} // namespace mepoo
} // namespace iox
