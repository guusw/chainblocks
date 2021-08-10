add_subdirectory(filesystem)
add_subdirectory(json)
add_subdirectory(kcp)
add_subdirectory(magic_enum)
add_subdirectory(nameof)
add_subdirectory(pareto)
add_subdirectory(replxx)
add_subdirectory(SDL)
add_subdirectory(spdlog)
add_subdirectory(wasm3)
add_subdirectory(Catch2)

option(SNAPPY_BUILD_TESTS OFF "")
add_subdirectory(snappy)

set(BROTLI_DISABLE_TESTS ON)
add_subdirectory(brotli)

option(TINYGLTF_BUILD_LOADER_EXAMPLE OFF "") 
add_subdirectory(tinygltf)

option(TF_BUILD_TESTS "" OFF)
option(TF_BUILD_EXAMPLES "" OFF)
add_subdirectory(cpp-taskflow)

option(KISSFFT_TEST "" OFF)
option(KISSFFT_TOOLS "" OFF)
add_subdirectory(kissfft)

option(BGFX_BUILD_EXAMPLES "" OFF)
option(BGFX_INSTALL "" OFF)
set(BX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bx)
set(BIMG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bimg)
set(BGFX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bgfx)
add_subdirectory(bgfx.cmake)

add_library(bgfx-example-common INTERFACE)
target_include_directories(bgfx-example-common INTERFACE ${BGFX_DIR}/examples/common/imgui)

if(APPLE)
  if(X86 OR X86_IOS_SIMULATOR)
    set(boost_context_SOURCES
        boost-context/src/posix/stack_traits.cpp
        boost-context/src/asm/jump_x86_64_sysv_macho_gas.S
        boost-context/src/asm/make_x86_64_sysv_macho_gas.S
        boost-context/src/asm/ontop_x86_64_sysv_macho_gas.S
      )
  else()
    set(boost_context_SOURCES
        boost-context/src/posix/stack_traits.cpp
        boost-context/src/asm/jump_arm64_aapcs_macho_gas.S
        boost-context/src/asm/make_arm64_aapcs_macho_gas.S
        boost-context/src/asm/ontop_arm64_aapcs_macho_gas.S
      )
  endif()
elseif(WIN32)
  set(boost_context_SOURCES
    boost-context/src/windows/stack_traits.cpp
    boost-context/src/asm/jump_x86_64_ms_pe_gas.asm
    boost-context/src/asm/make_x86_64_ms_pe_gas.asm
    boost-context/src/asm/ontop_x86_64_ms_pe_gas.asm
  )
endif()

enable_language(ASM-ATT)
add_library(boost-context ${boost_context_SOURCES})
target_include_directories(boost-context PUBLIC boost-context/include)

file(GLOB xxHash_SOURCES
  xxHash/*.c
)
add_library(xxHash ${xxHash_SOURCES})
target_include_directories(xxHash PUBLIC xxHash)

# file(GLOB backtrace_SOURCES
#   libbacktrace/*.c
# )
# add_library(backtrace ${backtrace_SOURCES})
# add_subdirectory(libbacktrace)

add_library(imgui_club INTERFACE)
target_include_directories(imgui_club INTERFACE imgui_club/imgui_memory_editor)

add_library(implot STATIC 
  implot/implot.cpp
  implot/implot_items.cpp
)
target_include_directories(implot PUBLIC implot)
target_link_libraries(implot
  PUBLIC dear-imgui
  PRIVATE stb
)
# target_include_directories(implot PUBLIC bgfx/3rdparty/dear-imgui)

add_library(linalg INTERFACE)
target_include_directories(linalg INTERFACE linalg)

add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE miniaudio)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE stb)

add_library(utf8.h INTERFACE)
target_include_directories(utf8.h INTERFACE utf8.h)

add_library(pdqsort INTERFACE) 
target_include_directories(pdqsort INTERFACE pdqsort)
