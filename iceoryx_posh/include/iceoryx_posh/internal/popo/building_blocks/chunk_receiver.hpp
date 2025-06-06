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
#ifndef IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_RECEIVER_HPP
#define IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_RECEIVER_HPP

#include "iceoryx_posh/internal/popo/building_blocks/chunk_queue_popper.hpp"
#include "iceoryx_posh/internal/popo/building_blocks/chunk_receiver_data.hpp"
#include "iceoryx_posh/mepoo/chunk_header.hpp"
#include "iox/expected.hpp"
#include "iox/not_null.hpp"

namespace iox
{
namespace popo
{
enum class ChunkReceiveResult
{
    TOO_MANY_CHUNKS_HELD_IN_PARALLEL,
    NO_CHUNK_AVAILABLE
};

/// @brief Converts the ChunkReceiveResult to a string literal
/// @param[in] value to convert to a string literal
/// @return pointer to a string literal
inline constexpr const char* asStringLiteral(const ChunkReceiveResult value) noexcept;

/// @brief Convenience stream operator to easily use the 'asStringLiteral' function with std::ostream
/// @param[in] stream sink to write the message to
/// @param[in] value to convert to a string literal
/// @return the reference to 'stream' which was provided as input parameter
inline std::ostream& operator<<(std::ostream& stream, ChunkReceiveResult value) noexcept;

/// @brief Convenience stream operator to easily use the 'asStringLiteral' function with iox::log::LogStream
/// @param[in] stream sink to write the message to
/// @param[in] value to convert to a string literal
/// @return the reference to 'stream' which was provided as input parameter
inline log::LogStream& operator<<(log::LogStream& stream, ChunkReceiveResult value) noexcept;

/// @brief The ChunkReceiver is a building block of the shared memory communication infrastructure. It extends
/// the functionality of a ChunkQueuePopper with the abililty to pass chunks to the user side (user process).
/// Together with the ChunkSender, they are the next abstraction layer on top of ChunkDistributor and ChunkQueuePopper.
/// The
/// ChunkRceiver holds the ownership of the SharedChunks and does a bookkeeping which chunks are currently passed to the
/// user side.
template <typename ChunkReceiverDataType>
class ChunkReceiver : public ChunkQueuePopper<typename ChunkReceiverDataType::ChunkQueueData_t>
{
  public:
    using MemberType_t = ChunkReceiverDataType;
    using Base_t = ChunkQueuePopper<typename ChunkReceiverDataType::ChunkQueueData_t>;

    explicit ChunkReceiver(not_null<MemberType_t* const> chunkReceiverDataPtr) noexcept;

    ChunkReceiver(const ChunkReceiver& other) = delete;
    ChunkReceiver& operator=(const ChunkReceiver&) = delete;
    ChunkReceiver(ChunkReceiver&& rhs) noexcept = default;
    ChunkReceiver& operator=(ChunkReceiver&& rhs) noexcept = default;
    ~ChunkReceiver() noexcept = default;

    /// @brief Tries to get the next received chunk. If there is a new one the ChunkHeader of this new chunk is received
    /// The ownerhip of the SharedChunk remains in the ChunkReceiver for being able to cleanup if the user process
    /// disappears
    /// @return New chunk header, ChunkReceiveResult on error
    /// or if there are no new chunks in the underlying queue
    expected<std::vector<mepoo::ChunkHeader*>, ChunkReceiveResult> tryGetChunkHeaders() noexcept;

    /// @brief Release a chunk that was obtained with get
    /// @param[in] chunkHeader, pointer to the ChunkHeader to release
    void release(std::vector<mepoo::ChunkHeader*>& chunkHeaders) noexcept;

    /// @brief Release all the chunks that are currently held. Caution: Only call this if the user process is no more
    /// running E.g. This cleans up chunks that were held by a user process that died unexpectetly, for avoiding lost
    /// chunks in the system
    void releaseAll() noexcept;

  private:
    const MemberType_t* getMembers() const noexcept;
    MemberType_t* getMembers() noexcept;
};

} // namespace popo
} // namespace iox

#include "iceoryx_posh/internal/popo/building_blocks/chunk_receiver.inl"

#endif // IOX_POSH_POPO_BUILDING_BLOCKS_CHUNK_RECEIVER_HPP
