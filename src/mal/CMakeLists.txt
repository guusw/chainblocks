set(chainblocks_SOURCES  
  stepA_mal.cpp
  Core.cpp
  CBCore.cpp
  Environment.cpp
  Reader.cpp
  ReadLine.cpp
  String.cpp
  Types.cpp
  Validation.cpp
  CBCore.cpp
)

add_executable(cbl ${chainblocks_SOURCES})
target_link_libraries(cbl
  chainblocks-core
  replxx
)

if(CHAINBLOCKS_WITH_EXTRA_BLOCKS)
  target_link_libraries(cbl chainblocks-extra)
endif()

if(EMSCRIPTEN)
  if(EMSCRIPTEN_PTHREADS)
    set_target_properties(cbl PROPERTIES SUFFIX "-mt.js")
  else()
    set_target_properties(cbl PROPERTIES SUFFIX "-st.js")
  endif()

  target_compile_options(cbl PRIVATE
    "SHELL:-s NO_EXIT_RUNTIME=1"
    "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
    "SHELL:-s INITIAL_MEMORY=209715200"
    "SHELL:-s FETCH=1"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:-s ASYNCIFY=1"
    "SHELL:-s LLD_REPORT_UNDEFINED"
    "SHELL:-s MODULARIZE=1"
    "SHELL:-s EXPORT_NAME=cbl"
    "SHELL:-s \"EXPORTED_FUNCTIONS=['_main', '_chainblocksInterface', '_malloc', '_free', '_emscripten_get_now']\""
    "SHELL:-s \"ASYNCIFY_IMPORTS=['emEval', 'emCompileShaderBlocking']\""
    "SHELL:-s \"EXPORTED_RUNTIME_METHODS=['FS', 'callMain', 'ENV', 'IDBFS', 'GL', 'PThread', 'setValue', 'getValue', 'lengthBytesUTF8', 'stringToUTF8']\""
  )
endif()