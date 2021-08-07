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
    # blocks/os.cpp
    blocks/process.cpp
    blocks/network.cpp
    blocks/ws.cpp
)

macro(add_chainblocks_library NAME LINK_TYPE)
  add_library(
    ${NAME} ${LINK_TYPE}
    ${core_SOURCES}
  )
  
  if(${LINK_TYPE} STREQUAL "SHARED")
    target_compile_definitions(${NAME} PUBLIC "-DCHAINBLOCKS_DLL=1")
  endif()
  target_include_directories(${NAME} PUBLIC ../../include)
  target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  # target_precompile_headers(${NAME} PRIVATE pch.hpp)
  
  target_link_libraries(${NAME} 
    PUBLIC spdlog magic_enum nameof
    PRIVATE pdqsort utf8.h Taskflow linalg stb nlohmann_json m3 ghc_filesystem boost-context boost_filesystem-mt
  )
  
  if(NOT EMSCRIPTEN)
    target_sources(${NAME} PRIVATE ${os_SOURCES})
  endif()
  
  if(WIN32)
    target_link_libraries(${NAME} PRIVATE ssl crypto ws2_32 mswsock Secur32 Crypt32 NCrypt)
  elseif(APPLE)
    target_link_directories(${NAME} PRIVATE
      /usr/local/lib
      /usr/local/opt/openssl/lib
    )
    target_include_directories(${NAME} PRIVATE
      /usr/local/include
      /usr/local/opt/openssl/include
      /usr/local/include/boost
    )
    target_link_libraries(${NAME} PRIVATE ssl crypto)
  elseif(UNIX)
    target_link_libraries(${NAME} PRIVATE ssl crypto pthread dl rt)
  endif()
  
  target_compile_options(${NAME} 
    PRIVATE -ffast-math -fno-finite-math-only -funroll-loops
  )
  
  target_compile_options(${NAME} 
    PUBLIC -Wno-multichar
    PRIVATE -Wno-missing-field-initializers -Wno-deprecated-copy
)
endmacro()
add_chainblocks_library(chainblocks-core SHARED)
add_chainblocks_library(chainblocks-core-static STATIC)

set(cbedn_SOURCES
  edn/main.cpp
  edn/eval.cpp
)
add_executable(cbedn ${cbedn_SOURCES})
target_link_libraries(cbedn chainblocks-core-static)