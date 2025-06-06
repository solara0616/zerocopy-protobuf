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
#ifndef IOX_POSH_MEPOO_SHARED_MULTI_CHUNK_HPP
#define IOX_POSH_MEPOO_SHARED_MULTI_CHUNK_HPP

#include "iceoryx_posh/internal/mepoo/chunk_management.hpp"
#include "iceoryx_posh/internal/mepoo/chunk_management_management.hpp"
#include "iceoryx_posh/internal/mepoo/mem_pool.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iox/relative_pointer.hpp"

namespace iox
{
namespace mepoo
{
template <typename>
class SharedPointer;

/// @brief WARNING: SharedChunk is not thread safe! Don't share SharedChunk objects between threads! Use for each thread
/// a separate copy
class SharedMultiChunk
{
  public:
    SharedMultiChunk() noexcept = default;
    SharedMultiChunk(ChunkManagementManagement* const resource) noexcept;
    ~SharedMultiChunk() noexcept;

    SharedMultiChunk(const SharedMultiChunk& rhs) noexcept;
    SharedMultiChunk(SharedMultiChunk&& rhs) noexcept;

    SharedMultiChunk& operator=(const SharedMultiChunk& rhs) noexcept;
    SharedMultiChunk& operator=(SharedMultiChunk&& rhs) noexcept;

    ChunkHeader* getChunkHeader() const noexcept;
    std::vector<ChunkHeader*> getChunkHeaders() const noexcept;
    void* getUserPayload() const noexcept;

    ChunkManagementManagement* release() noexcept;
    ChunkManagement* getFirstChunkManagement() noexcept;
    ChunkManagementManagement* getChunkManagentManagement()
    {
      return m_chunkManagementManagement;
    }

    bool addChunkManagement(const not_null<ChunkManagement*> chunkManagement) noexcept;

    bool operator==(const SharedMultiChunk& rhs) const noexcept;
    /// @todo iox-#1617 use the newtype pattern to avoid the void pointer
    bool operator==(const void* const rhs) const noexcept;

    bool operator!=(const SharedMultiChunk& rhs) const noexcept;
    bool operator!=(const void* const rhs) const noexcept;

    explicit operator bool() const noexcept;

    template <typename>
    friend class SharedPointer;

  private:
    void decrementReferenceCounter() noexcept;
    void incrementReferenceCounter() noexcept;

  private:
    ChunkManagementManagement* m_chunkManagementManagement{nullptr};
};
} // namespace mepoo
} // namespace iox

#endif // IOX_POSH_MEPOO_SHARED_MULTI_CHUNK_HPP
