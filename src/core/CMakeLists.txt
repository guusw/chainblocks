set(core_SOURCES
  runtime.cpp
  ops_internal.cpp
  blocks/assert.cpp
  blocks/chains.cpp
  blocks/logging.cpp
  blocks/flow.cpp
  blocks/seqs.cpp
  blocks/casting.cpp
  blocks/core.cpp
  blocks/linalg.cpp
  blocks/serialization.cpp
  blocks/json.cpp
  blocks/struct.cpp
  blocks/time.cpp
  blocks/strings.cpp
  blocks/channels.cpp
  blocks/random.cpp
  blocks/bigint.cpp
  blocks/fs.cpp
  blocks/edn.cpp
  blocks/reflection.cpp
  blocks/imaging.cpp
  blocks/wasm.cpp
  blocks/http.cpp
)

set(os_SOURCES
  blocks/process.cpp
  blocks/network.cpp
  blocks/ws.cpp
)

option(CHAINBLOCKS_CORE_BUILD_SHARED OFF "Build chainblocks core as dynamic library")
if(CHAINBLOCKS_CORE_BUILD_SHARED)
  add_library(chainblocks-core SHARED ${core_SOURCES})
  target_compile_definitions(chainblocks-core PUBLIC "-DCHAINBLOCKS_CORE_DLL=1")
else()
  add_library(chainblocks-core STATIC ${core_SOURCES})
endif()

target_include_directories(chainblocks-core PUBLIC ../../include)
target_include_directories(chainblocks-core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# target_precompile_headers(chainblocks-core PRIVATE pch.hpp)

target_link_libraries(chainblocks-core 
  PUBLIC spdlog magic_enum nameof linalg
  PRIVATE pdqsort utf8.h Taskflow stb nlohmann_json m3 ghc_filesystem boost-context boost_filesystem-mt
)

if(NOT EMSCRIPTEN)
  target_sources(chainblocks-core PRIVATE ${os_SOURCES})
endif()

if(WIN32)
  target_link_libraries(chainblocks-core PRIVATE ssl crypto ws2_32 mswsock Secur32 Crypt32 NCrypt)
elseif(APPLE)
  target_link_directories(chainblocks-core PRIVATE
    /usr/local/lib
    /usr/local/opt/openssl/lib
  )
  target_include_directories(chainblocks-core PRIVATE
    /usr/local/include
    /usr/local/opt/openssl/include
    /usr/local/include/boost
  )
  target_link_libraries(chainblocks-core PRIVATE ssl crypto)
elseif(UNIX)
  target_link_libraries(chainblocks-core PRIVATE ssl crypto pthread dl rt)
endif()

target_compile_options(chainblocks-core 
  PRIVATE -ffast-math -fno-finite-math-only -funroll-loops
)

target_compile_options(chainblocks-core 
  PUBLIC -Wno-multichar
  PRIVATE -Wno-missing-field-initializers -Wno-deprecated-copy
)

# TODO: Modularize block registration to remove Core=>Rust dependency
if(CHAINBLOCKS_WITH_EXTRA_BLOCKS)
  target_compile_definitions(chainblocks-core PUBLIC "-DCHAINBLOCKS_WITH_EXTRA_BLOCKS=1")
  target_link_libraries(chainblocks-core PRIVATE chainblocks-extra)
endif()

set(cbedn_SOURCES
  edn/main.cpp
  edn/eval.cpp
)
add_executable(cbedn ${cbedn_SOURCES})
target_link_libraries(cbedn chainblocks-core)

if(CHAINBLOCKS_BUILD_TESTS)
  add_executable(chainblocks-tests-runtime ../tests/test_runtime.cpp)
  target_link_libraries(chainblocks-tests-runtime 
    chainblocks-core Catch2WithMain
  )
  catch_discover_tests(chainblocks-tests-runtime)
endif()