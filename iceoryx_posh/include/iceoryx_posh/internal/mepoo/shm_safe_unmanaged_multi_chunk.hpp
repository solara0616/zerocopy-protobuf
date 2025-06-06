// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
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

#ifndef IOX_POSH_MEPOO_SHM_SAFE_UNMANAGED_MULTI_CHUNK_HPP
#define IOX_POSH_MEPOO_SHM_SAFE_UNMANAGED_MULTI_CHUNK_HPP

#include "iceoryx_posh/internal/mepoo/shared_chunk.hpp"
#include "iceoryx_posh/internal/mepoo/shared_multi_chunk.hpp"
#include "iox/detail/relative_pointer_data.hpp"

#include <vector>

namespace iox
{
namespace mepoo
{
/// @brief This class to safely store a chunk in shared memory. To be able to do so, torn writes/reads need to
/// prevented, since they create Frankenstein objects. Therefore, the class must not be larger than 64 bits and
/// trivially copy-able in case an application dies while writing this and RouDi needs to clean up.
class ShmSafeUnmanagedMultiChunk
{
  public:
    ShmSafeUnmanagedMultiChunk() noexcept = default;

    ShmSafeUnmanagedMultiChunk(ChunkManagementManagement* chunkManagementManagement) noexcept;

    /// @brief takes a SharedMultiChunk without decrementing the chunk reference counter
    ShmSafeUnmanagedMultiChunk(SharedMultiChunk chunk) noexcept;

    /// @brief Creates a SharedChunk without incrementing the chunk reference counter and invalidates itself
    SharedMultiChunk releaseToSharedChunk() noexcept;

    /// @brief Creates a SharedMultiChunk with incrementing the chunk reference counter and does not invalidate itself
    SharedMultiChunk cloneToSharedChunk() noexcept;

    SharedChunk cloneFirstToSharedChunk() noexcept;

    SharedChunk releaseFirstToSharedChunk() noexcept;

    /// @brief Checks if the underlying RelativePointerData to the chunk is logically a nullptr
    /// @return true if logically a nullptr otherwise false
    bool isLogicalNullptr() const noexcept;

    /// @brief Access to the ChunkHeader of the underlying chunk
    /// @return the pointer to the ChunkHeader of the underlying chunk or nullptr if isLogicalNullptr would return true
    ChunkHeader* getChunkHeader() noexcept;

    std::vector<ChunkHeader*> getChunkHeaders() noexcept;

    /// @brief const access to the ChunkHeader of the underlying chunk
    /// @return the const pointer to the ChunkHeader of the underlying chunk or nullptr if isLogicalNullptr would return
    /// true
    const ChunkHeader* getChunkHeader() const noexcept;

    ChunkManagementManagement* getChunkManagementManagement() noexcept;

    /// @brief Checks if the underlying RelativePointerData to the chunk is neither logically a nullptr nor that the
    /// chunk has other owner
    /// @return true if neither logically a nullptr nor other owner chunk owners present, otherwise false
    bool isNotLogicalNullptrAndHasNoOtherOwners() const noexcept;

    bool addChunkManagement(const not_null<ChunkManagement*> chunkManagement) noexcept;

  private:
    RelativePointerData m_chunkManagementManagement;
};

} // namespace mepoo
} // namespace iox

#endif // IOX_POSH_MEPOO_SHM_SAFE_UNMANAGED_CHUNK_HPP
