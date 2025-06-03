// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
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
#ifndef IOX_POSH_MEPOO_CHUNK_MANAGEMENT_MANAGEMENT_HPP
#define IOX_POSH_MEPOO_CHUNK_MANAGEMENT_MANAGEMENT_HPP

#include "iox/atomic.hpp"
#include "iox/fixed_position_container.hpp"
#include "iox/not_null.hpp"
#include "iox/relative_pointer.hpp"
#include "iceoryx_posh/internal/mepoo/chunk_management.hpp"

#include <cstdint>

namespace iox
{
namespace mepoo
{
class MemPool;
struct ChunkHeader;

constexpr uint32_t MAX_CHUNK_NUMBER_IN_ONE_REQ = 3;

struct ChunkManagementManagement
{
    using base_t = ChunkManagement;
    using referenceCounterBase_t = uint64_t;
    using element_t = iox::RelativePointer<base_t>;
    using referenceCounter_t = concurrent::Atomic<referenceCounterBase_t>;
    using chunkManagementContainer_t = FixedPositionContainer<element_t, MAX_CHUNK_NUMBER_IN_ONE_REQ>;

    ChunkManagementManagement(const not_null<MemPool*> chunkManagementPool) noexcept;

    referenceCounter_t m_referenceCounter{1U};
    chunkManagementContainer_t m_chunkManagements;
    iox::RelativePointer<MemPool> m_chunkManagementPool;

    bool addChunkManagement(const not_null<ChunkManagement*> chunkManagement) noexcept;
};
} // namespace mepoo
} // namespace iox

#endif // IOX_POSH_MEPOO_CHUNK_MANAGEMENT_MANAGEMENT_HPP
