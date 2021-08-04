set(core_SOURCES
  runtime.cpp

add_library(cb_runtime SHARED
  runtime.cpp
  ops_internal.cpp
)
target_compile_definitions(${NAME} PRIVATE "-DCB_DLL_EXPORT")
target_include_directories(${NAME} PUBLIC ../../include)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${NAME} PRIVATE spdlog)
target_link_libraries(${NAME} PRIVATE magic_enum)
target_link_libraries(${NAME} PRIVATE nameof)
target_link_libraries(${NAME} PRIVATE pdqsort)
target_link_libraries(${NAME} PRIVATE utf8.h)
target_link_libraries(${NAME} PRIVATE Taskflow)
target_link_libraries(${NAME} PRIVATE linalg)
target_link_libraries(${NAME} PRIVATE stb)
target_link_libraries(${NAME} PRIVATE nlohmann_json)
target_link_libraries(${NAME} PRIVATE m3)
target_link_libraries(${NAME} PRIVATE ghc_filesystem)
target_link_libraries(${NAME} PRIVATE boost-context)
  
# set(core_SOURCES
#   runtime.cpp
#   ops_internal.cpp
#   blocks/assert.cpp
#   blocks/chains.cpp
#   blocks/logging.cpp
#   blocks/flow.cpp
#   blocks/seqs.cpp
#   blocks/casting.cpp
#   blocks/core.cpp
#   blocks/linalg.cpp
#   blocks/serialization.cpp
#   blocks/json.cpp
#   blocks/struct.cpp
#   blocks/time.cpp
#   blocks/strings.cpp
#   blocks/channels.cpp
#   blocks/random.cpp
#   # blocks/imaging.cpp
#   blocks/bigint.cpp
#   blocks/fs.cpp
#   # blocks/wasm.cpp
#   blocks/edn.cpp
#   blocks/reflection.cpp
#   # blocks/http.cpp
# )

# set(os_SRC
#     blocks/process.cpp
#     blocks/os.cpp
#     blocks/network.cpp
#     blocks/ws.cpp
# )

# macro(cb_multi NAME LINK_TYPE)
#   add_library(
#     ${NAME} ${LINK_TYPE}
#     ${core_SOURCES}
#   )
#   target_compile_definitions(${NAME} PRIVATE "-DCB_DLL_EXPORT")
#   target_include_directories(${NAME} PUBLIC ../../include)
#   target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
#   target_link_libraries(${NAME} PRIVATE spdlog)
#   target_link_libraries(${NAME} PRIVATE magic_enum)
#   target_link_libraries(${NAME} PRIVATE nameof)
#   target_link_libraries(${NAME} PRIVATE pdqsort)
#   target_link_libraries(${NAME} PRIVATE utf8.h)
#   target_link_libraries(${NAME} PRIVATE Taskflow)
#   target_link_libraries(${NAME} PRIVATE linalg)
#   target_link_libraries(${NAME} PRIVATE stb)
#   target_link_libraries(${NAME} PRIVATE nlohmann_json)
#   target_link_libraries(${NAME} PRIVATE m3)
#   target_link_libraries(${NAME} PRIVATE ghc_filesystem)
#   target_link_libraries(${NAME} PRIVATE boost-context)
  
#   target_compile_options(${NAME} PRIVATE 
#     -ffast-math
#     -fno-finite-math-only
#     -funroll-loops
#   )
#   target_compile_options(${NAME} PRIVATE 
#     -Wno-multichar
#     -Wno-missing-field-initializers
#     -Wno-deprecated-copy
# )
# endmacro()
# cb_multi(cb-core SHARED)