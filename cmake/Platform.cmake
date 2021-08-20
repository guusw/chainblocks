if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  add_compile_definitions(CPUBITS64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  add_compile_definitions(CPUBITS32)
endif()

if(EMSCRIPTEN)
  set(EM_EXPORTED_FUNCTIONS "-s \"EXPORTED_FUNCTIONS=['_main', '_chainblocksInterface', '_malloc', '_free', '_emscripten_get_now']\"")
  set(EM_ASYNCIFY_IMPORTS "-s \"ASYNCIFY_IMPORTS=['emEval', 'emCompileShaderBlocking']\"")
  set(EM_EXPORTED_RUNTIME_METHODS "-s \"EXPORTED_RUNTIME_METHODS=['FS', 'callMain', 'ENV', 'IDBFS', 'GL', 'PThread', 'setValue', 'getValue', 'lengthBytesUTF8', 'stringToUTF8']\"")

  # MinSizeRel is better for release builds and RelWithDebInfo for debug
  # check github actions CI for config command line
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s NO_EXIT_RUNTIME=1 -s DISABLE_EXCEPTION_CATCHING=0 -s INITIAL_MEMORY=209715200 -s FETCH=1 -s ALLOW_MEMORY_GROWTH=1 -s ASYNCIFY=1 -s LLD_REPORT_UNDEFINED -s MODULARIZE=1 -s EXPORT_NAME=cbl -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s USE_SDL=2 ${EM_EXPORTED_FUNCTIONS} ${EM_ASYNCIFY_IMPORTS} ${EM_EXPORTED_RUNTIME_METHODS}")
  
  if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ASSERTIONS=2")
    add_compile_options(-g1 -Os)
  endif()
  
  add_compile_definitions(NO_FORCE_INLINE)
  add_link_options(--bind)
  
  ## if we wanted thread support...
  if(EMSCRIPTEN_PTHREADS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=6")
    add_compile_options(-pthread -Wno-pthreads-mem-growth)
    add_link_options(-pthread)
  endif()
  
  if(EMSCRIPTEN_IDBFS)
    add_link_options(-lidbfs.js)
  endif()
  
  if(EMSCRIPTEN_NODEFS)
    add_link_options(-lnodefs.js)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  # aggressive inlining
  if(NOT SKIP_HEAVY_INLINE)
    # this works with emscripten too but makes the final binary much bigger
    # for now let's keep it disabled
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      set(INLINING_FLAGS -mllvm -inline-threshold=100000)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
      set(INLINING_FLAGS -finline-limit=100000)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
      # using Intel C++
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
      # using Visual Studio C++
    endif()
  endif()
endif()

if(USE_LTO)
  add_compile_options(-flto)
  add_link_options(-flto)
endif()

add_compile_options(-ffast-math -fno-finite-math-only -funroll-loops -Wno-multichar)

if((WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug") OR FORCE_USE_LLD)
  add_link_options(-fuse-ld=lld)
  SET(CMAKE_AR llvm-ar)
  SET(CMAKE_RANLIB llvm-ranlib)
endif()

if(USE_GCC_ANALYZER)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fanalyzer)
  endif()
endif()

if(PROFILE_GPROF)
  add_compile_options(-pg -DNO_FORCE_INLINE)
  add_link_options(-pg)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(NOT WIN32 AND NOT EMSCRIPTEN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
  endif()
endif()

if(CODE_COVERAGE)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fprofile-arcs -ftest-coverage -fno-default-inline -fno-inline)
    add_link_options(-fprofile-arcs -ftest-coverage -fno-default-inline -fno-inline)
  endif()
endif()

# Move this?
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
  else()
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
  endif()
else()
  add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
endif()

# Move this?
if(VERBOSE_BGFX_LOGGING)
  add_compile_definitions(BGFX_CONFIG_DEBUG)
endif()

add_compile_definitions(BOOST_INTERPROCESS_BOOTSTAMP_IS_LASTBOOTUPTIME=1)
add_compile_options(-Wall)

# this works with emscripten too but makes the final binary much bigger
if(NOT SKIP_HEAVY_INLINE)
  if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      add_compile_options(-mllvm -inline-threshold=100000)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
      add_compile_options(-finline-limit=100000)
    endif()
  endif()
endif()
