# Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
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
cmake_minimum_required(VERSION 3.16)
project(protobuf-build CXX)

include(ProcessorCount)
ProcessorCount(N)

if(NOT N EQUAL 0)
    if(((${CMAKE_VERSION} VERSION_GREATER "3.12.0") OR ${CMAKE_VERSION} VERSION_EQUAL "3.12.0"))
        set(CMAKE_BUILD_FLAGS -j ${N})
    elseif(LINUX OR QNX)
        set(CMAKE_BUILD_FLAGS -- -j ${N})
    endif()
endif()

if(WIN32)
    set(CREATE_PATH_COMMAND mkdir)
else()
    set(CREATE_PATH_COMMAND mkdir -p)
endif()

# set build paths
set(SOURCE_DIR ${CMAKE_BINARY_DIR}/../iceoryx_posh/protobuf-3.19.3)
set(BUILD_DIR ${CMAKE_BINARY_DIR}/dependencies/protobuf/build)
set(INSTALL_DIR ${CMAKE_BINARY_DIR}/dependencies/install)

file(MAKE_DIRECTORY ${BUILD_DIR})

set(CMAKE_ADDITIONAL_OPTIONS
    "-DCMAKE_C_FLAGS_INIT=${CMAKE_C_FLAGS_INIT}"
    "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}"
    "-DCMAKE_CXX_FLAGS_INIT=${CMAKE_CXX_FLAGS_INIT}"
    "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
    "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
    "-DCMAKE_C_COMPILER_TARGET=${CMAKE_C_COMPILER_TARGET}"
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_CXX_COMPILER_TARGET=${CMAKE_CXX_COMPILER_TARGET}"
    "-DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
    "-Dprotobuf_BUILD_TESTS=OFF"
    "-DCMAKE_LINKER=${CMAKE_LINKER}")

if(DEFINED CMAKE_TOOLCHAIN_FILE)
    list(APPEND CMAKE_ADDITIONAL_OPTIONS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
    set(cppprotobuf_DIR ${CMAKE_BINARY_DIR}/dependencies/install/lib/cmake/protobuf)
    set(cppprotobuf_DIR ${cpptoml_DIR} CACHE PATH "" FORCE)
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" "-DENABLE_LIBCXX=off" "-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}" "${SOURCE_DIR}/cmake" ${CMAKE_ADDITIONAL_OPTIONS}
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${BUILD_DIR} )
if(result)
    message(FATAL_ERROR "CMake step [configure] for protobuf failed: ${result}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} --build . --target install ${CMAKE_BUILD_FLAGS}
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${BUILD_DIR} )
if(result)
    message(FATAL_ERROR "Build step [build and install] for protobuf failed: ${result}")
endif()

list(APPEND CMAKE_PREFIX_PATH ${INSTALL_DIR})
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} CACHE INTERNAL "" FORCE)

