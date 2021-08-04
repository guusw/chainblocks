cmake_minimum_required(VERSION 3.14)

project("chainblocks")

# Disable CTest for any subprojects by default
option(BUILD_TESTING "" OFF)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
  set (X86 TRUE)
else()
  set (X86 FALSE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  set(EXTERNAL_BUILD_TYPE "MinSizeRel")
else()
  set(EXTERNAL_BUILD_TYPE "Release")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Sanitizers)
include(Platform)
# include(Extra)

add_subdirectory(deps)
add_subdirectory(src/core)