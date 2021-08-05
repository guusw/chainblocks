set(extra_SOURCES
  runtime.cpp
  bgfx.cpp
  bgfx.shaderc.cpp
  imgui.cpp
  xr.cpp
  dsp.cpp
  gltf.cpp
  inputs.cpp
)

add_library(chainblocks-extra SHARED ${extra_SOURCES})
target_link_libraries(chainblocks-extra
  PRIVATE stb
  PRIVATE brotli
  PRIVATE gltf
  PRIVATE imgui
  PRIVATE bx
  PRIVATE bimg
  PRIVATE bgfx
  PRIVATE snappy
)