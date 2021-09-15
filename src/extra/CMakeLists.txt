set(extra_SOURCES
  runtime.cpp
  bgfx.cpp
  bgfx.shaderc.cpp
  imgui.cpp
  xr.cpp
  dsp.cpp
  gltf.cpp
  inputs.cpp
  snappy.cpp
  brotli.cpp
  audio.cpp
)

if(CHAINBLOCKS_EXTRA_BUILD_SHARED)
  add_library(chainblocks-extra SHARED ${extra_SOURCES})
  target_compile_definitions(chainblocks-extra PUBLIC "-DCHAINBLOCKS_EXTRA_DLL=1")
else()
  add_library(chainblocks-extra STATIC ${extra_SOURCES})
endif()

target_include_directories(chainblocks-extra PRIVATE ../core)
target_link_libraries(chainblocks-extra
  chainblocks-core 
  bx bimg bgfx bgfx-example-common stb tinygltf
  dear-imgui imgui_club implot
  brotlienc-static brotlidec-static brotlicommon-static snappy
  kissfft miniaudio
  nlohmann_json
)

if(EMSCRIPTEN)
  target_link_libraries(chainblocks-extra SDL2)
else()
  # Use builtin SDL2 port
  target_compile_options(chainblocks-extra PRIVATE 
    "SHELL:-s MIN_WEBGL_VERSION=2"
    "SHELL:-s MAX_WEBGL_VERSION=2"
    "SHELL:-s USE_SDL=2")
  target_link_libraries(chainblocks-extra SDL2)
endif()

if(WIN32)
  target_link_libraries(chainblocks-extra DXGI D3D11 ntdll)
  target_sources(chainblocks-extra PRIVATE desktop.win.cpp)
endif()

option(CHAINBLOCKS_EXTRA_BUILD_SHARED OFF "Build chainblocks extra as dynamic library")

if(CHAINBLOCKS_WITH_RUST_BLOCKS)
  target_compile_definitions(chainblocks-extra PUBLIC "-DCHAINBLOCKS_WITH_RUST_BLOCKS=1")
  target_link_libraries(chainblocks-extra chainblocks-rust)
endif()

if(EMSCRIPTEN)
  em_link_js_library(chainblocks-extra bgfx.shaderc.js)
endif()
