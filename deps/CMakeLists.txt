# include(ExternalProject)
# ExternalProject_Add(bgfx
#   SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bgfx.cmake
#   CMAKE_CACHE_ARGS
#     -DBX_DIR:STRING=${CMAKE_CURRENT_SOURCE_DIR}/bx
#     -DBIMG_DIR:STRING=${CMAKE_CURRENT_SOURCE_DIR}/bimg
#     -DBGFX_DIR:STRING=${CMAKE_CURRENT_SOURCE_DIR}/bgfx
#   INSTALL_COMMAND ""
# )
set(BX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bx)
set(BIMG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bimg)
set(BGFX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bgfx)
add_subdirectory(bgfx.cmake)

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
  add_library(boost-context ${boost_context_SOURCES}) 
endif()

add_executable(sandbox sandbox.cpp)
target_link_libraries(sandbox bx bimg bgfx)