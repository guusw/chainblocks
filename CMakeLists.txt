cmake_minimum_required(VERSION 3.14)

project("chainblocks")

option(CHAINBLOCKS_BUILD_TESTS "Enable to build chainblocks tests" ON)
option(CHAINBLOCKS_WITH_EXTRA_BLOCKS "Enabled to build and include the extra c++ blocks (graphics, audio, etc.)" ON)
option(CHAINBLOCKS_WITH_RUST_BLOCKS "Enable to build and include the extra rust blocks" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/chainblocks-cmake-shared")
get_filename_component(CHAINBLOCKS_DIR ${CMAKE_CURRENT_LIST_DIR} ABSOLUTE)

include(Platform)
include(Sanitizers)
include(CompileFlags)
include(Tidy)
include(Utils)

# Rust build support
include(Rust)

# disable targets being added by CTestTargets.cmake
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

include(Deps)

enable_testing()
include(CTest)
include(${Catch2_DIR}/Catch.cmake)

add_subdirectory(src/core)
add_subdirectory(src/mal)

if(CHAINBLOCKS_WITH_EXTRA_BLOCKS)
  if(CHAINBLOCKS_WITH_RUST_BLOCKS)
    add_subdirectory(rust)
  endif()
  
  add_subdirectory(src/extra)
endif()