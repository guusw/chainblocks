if(NOT CMAKE_PREFIX_PATH)
  set(CHAINBLOCKS_AUTO_DEPS_DEFAULT ON)
endif()

option(CHAINBLOCKS_AUTO_DEPS "Automatically download prebuilt dependencies" ${CHAINBLOCKS_AUTO_DEPS_DEFAULT})
if(CHAINBLOCKS_AUTO_DEPS) 
  set(AUTO_DEPS_BASE_PATH "${CHAINBLOCKS_DIR}/auto-deps")
  set(AUTO_DEPS_PATH "${AUTO_DEPS_BASE_PATH}/${TARGET_ID}")
  set(AUTO_DEPS_FILENAME ${TARGET_ID}.tar.gz)
  set(AUTO_DEPS_ARCHIVE_PATH ${AUTO_DEPS_BASE_PATH}/${AUTO_DEPS_FILENAME})
  
  if(NOT EXISTS ${AUTO_DEPS_ARCHIVE_PATH})
    set(DOWNLOAD_URL https://github.com/guusw/chainblocks-deps/releases/download/auto-deps-${TARGET_ID}/${AUTO_DEPS_FILENAME})
    message(STATUS "Downloading dependencies from ${DOWNLOAD_URL}")
    file(
      DOWNLOAD ${DOWNLOAD_URL} ${AUTO_DEPS_ARCHIVE_PATH}
      SHOW_PROGRESS
      STATUS DOWNLOAD_STATUS
    )
    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_STATUS_CODE)
    list(GET DOWNLOAD_STATUS 1 DOWNLOAD_STATUS_STRING)
    if(NOT DOWNLOAD_STATUS_CODE EQUAL 0)
      file(REMOVE ${AUTO_DEPS_ARCHIVE_PATH})
      message(FATAL_ERROR "Failed to download dependencies: ${DOWNLOAD_STATUS_STRING} (${DOWNLOAD_STATUS_CODE})")
    endif()
  endif()
  
  set(TEST_PATH ${AUTO_DEPS_PATH}/include)
  if(NOT EXISTS ${TEST_PATH})
    message(STATUS "Extracting downloaded archive ${AUTO_DEPS_ARCHIVE_PATH} into ${AUTO_DEPS_PATH}")
    file(ARCHIVE_EXTRACT
      INPUT ${AUTO_DEPS_ARCHIVE_PATH}
      DESTINATION ${AUTO_DEPS_PATH}
    )
    
    if(NOT EXISTS ${AUTO_DEPS_PATH}/include)
      file(REMOVE ${AUTO_DEPS_ARCHIVE_PATH})
      file(REMOVE_RECURSE ${AUTO_DEPS_PATH})
      message(FATAL_ERROR "Corrupt dependency archive, removing")
    endif()
  endif()
  
  list(REMOVE_ITEM CMAKE_PREFIX_PATH ${AUTO_DEPS_PATH})
  list(APPEND CMAKE_PREFIX_PATH ${AUTO_DEPS_PATH})
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} CACHE STRING "" FORCE)
  message(STATUS "Update CMAKE_PREFIX_PATH = ${CMAKE_PREFIX_PATH}")
  
  list(REMOVE_ITEM CMAKE_FIND_ROOT_PATH ${AUTO_DEPS_PATH})
  list(APPEND CMAKE_FIND_ROOT_PATH ${AUTO_DEPS_PATH})
  set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} CACHE STRING "" FORCE)
  message(STATUS "Update CMAKE_FIND_ROOT_PATH = ${CMAKE_FIND_ROOT_PATH}")
endif()

find_package(boost_core 1.78.0 REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
if(NOT EMSCRIPTEN)
  find_package(boost_context 1.78.0 REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
  find_package(boost_filesystem 1.78.0 REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
endif()
if(WIN32)
  find_package(boost_stacktrace_windbg 1.78.0 REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
endif()

if(NOT EMSCRIPTEN)
  find_package(SDL2 REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
endif()

find_package(nlohmann_json REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(kcp REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(magic_enum REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(nameof REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(Pareto REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(replxx REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(spdlog REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(wasm3 REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(Catch2 REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(snappy REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(brotli REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(tinygltf REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(Taskflow REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(kissfft REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(bgfx REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(xxHash REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(linalg REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(miniaudio REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(stb REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(utf8.h REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)
find_package(pdqsort REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)

add_library(imgui_club INTERFACE)
target_include_directories(imgui_club INTERFACE ${CHAINBLOCKS_DIR}/deps/imgui_club/imgui_memory_editor)

set(imguizmo_SOURCES 
  ${CHAINBLOCKS_DIR}/deps/imguizmo/ImCurveEdit.cpp
  ${CHAINBLOCKS_DIR}/deps/imguizmo/ImGradient.cpp
  ${CHAINBLOCKS_DIR}/deps/imguizmo/ImGuizmo.cpp
  ${CHAINBLOCKS_DIR}/deps/imguizmo/ImSequencer.cpp
)

add_library(imguizmo STATIC ${imguizmo_SOURCES})
target_include_directories(imguizmo PUBLIC ${CHAINBLOCKS_DIR}/deps/imguizmo)
target_link_libraries(imguizmo PUBLIC bgfx::dear-imgui)

add_library(implot STATIC 
  ${CHAINBLOCKS_DIR}/deps/implot/implot.cpp
  ${CHAINBLOCKS_DIR}/deps/implot/implot_items.cpp
)
target_include_directories(implot PUBLIC ${CHAINBLOCKS_DIR}/deps/implot)
target_link_libraries(implot
  PUBLIC bgfx::dear-imgui
  PRIVATE stb
)
