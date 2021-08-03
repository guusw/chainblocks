### libs
if(WIN32)
  set(BOOST_LIBS -lboost_context-mt -lboost_filesystem-mt -lssl -lcrypto -lws2_32 -lmswsock -lSecur32 -lCrypt32 -lNcrypt)
elseif(APPLE)
  if(NOT IOS)
    set(BOOST_LIBS -lboost_context-mt -lboost_filesystem-mt -lssl -lcrypto)
  endif()
  include_directories(
    /usr/local/include
    /usr/local/opt/openssl/include
    )
  link_directories(
    /usr/local/lib
    /usr/local/opt/openssl/lib
    )
elseif(EMSCRIPTEN)
  include_directories(${CHAINBLOCKS_DIR}/external/boost_1_73_0)
  include_directories(${CHAINBLOCKS_DIR}/external/boost_1_74_0)
  include_directories(${CHAINBLOCKS_DIR}/external/boost_1_75_0)
  include_directories(${CHAINBLOCKS_DIR}/external/boost_1_76_0)
elseif(UNIX)
  set(BOOST_LIBS -lboost_context -lboost_filesystem -lssl -lcrypto -pthread -ldl -lrt)
endif()

if(APPLE)
  add_compile_definitions(BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
  add_compile_options(-Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
  include_directories(/usr/local/include/boost)
endif()

if(UNIX)
  add_compile_options(-fvisibility=hidden)
endif()



if(LINUX)
  add_link_options(-export-dynamic)
  if(USE_VALGRIND)
    add_compile_definitions(BOOST_USE_VALGRIND)
  endif()
endif()

if(EMSCRIPTEN)
  set(EXTRA_CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
  if(EMSCRIPTEN_PTHREADS)
    set(EXTRA_CMAKE_ARGS ${EXTRA_CMAKE_ARGS} -DCMAKE_CXX_FLAGS=-sUSE_PTHREADS=1 -DCMAKE_C_FLAGS=-sUSE_PTHREADS=1)
  endif()
elseif(IOS)
  add_compile_definitions(CB_NO_HTTP_BLOCKS=1)
  set(EXTRA_CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME})
  if(X86_IOS_SIMULATOR)
    set(EXTRA_CMAKE_BUILD_ARGS -- -sdk iphonesimulator -arch x86_64)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(EXTRA_LIBS_DIR "Debug-iphonesimulator")
    else()
      set(EXTRA_LIBS_DIR "Release-iphonesimulator")
    endif()
  else()
    set(EXTRA_CMAKE_BUILD_ARGS -- -sdk iphoneos -arch arm64)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(EXTRA_LIBS_DIR "Debug-iphoneos")
    else()
      set(EXTRA_LIBS_DIR "Release-iphoneos")
    endif()
  endif()
endif()

### runtime
include_directories(
  ${CHAINBLOCKS_DIR}/deps/replxx/include
  ${CHAINBLOCKS_DIR}/deps/spdlog/include
  ${CHAINBLOCKS_DIR}/deps/SPSCQueue/include
  ${CHAINBLOCKS_DIR}/deps/stb
  ${CHAINBLOCKS_DIR}/deps/xxHash
  ${CHAINBLOCKS_DIR}/deps/json/include
  ${CHAINBLOCKS_DIR}/deps/magic_enum/include
  ${CHAINBLOCKS_DIR}/deps/nameof/include
  ${CHAINBLOCKS_DIR}/deps/kcp
  ${CHAINBLOCKS_DIR}/deps/cpp-taskflow
  ${CHAINBLOCKS_DIR}/include
  ${CHAINBLOCKS_DIR}/src/core
  ${CHAINBLOCKS_DIR}/deps/pdqsort
  ${CHAINBLOCKS_DIR}/deps/filesystem/include
  ${CHAINBLOCKS_DIR}/deps/wasm3/source
  ${CHAINBLOCKS_DIR}/deps/xxhash
  ${CHAINBLOCKS_DIR}/deps/linalg
  ${CHAINBLOCKS_DIR}/deps/pareto/source
  )

# wasm3
ExternalProject_Add(wasm3
  URL ${CHAINBLOCKS_DIR}/deps/wasm3
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/wasm3-deps
  BUILD_IN_SOURCE True
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=${EXTERNAL_BUILD_TYPE} ${EXTRA_CMAKE_ARGS} -DWASM3_ARCH_FLAGS=${ARCH_FLAGS} -DBUILD_NATIVE=0
  BUILD_COMMAND cmake --build . --target m3
  INSTALL_COMMAND ""
  BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/wasm3-deps/src/wasm3/source/libm3.a
)

add_library(libwasm3 STATIC IMPORTED GLOBAL)
set_target_properties(libwasm3 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/wasm3-deps/src/wasm3/source/libm3.a)
add_dependencies(libwasm3 wasm3)

if(NOT BUILD_CORE_ONLY)
  add_compile_definitions(
    CB_WITH_EXTRAS
    BIMG_DECODE_ENABLE=0
    SDL_RENDER_DISABLED=1
    )

  include_directories(
    ${CHAINBLOCKS_DIR}/deps/bx/include
    ${CHAINBLOCKS_DIR}/deps/bx/3rdparty
    ${CHAINBLOCKS_DIR}/deps/bgfx/include
    ${CHAINBLOCKS_DIR}/deps/bgfx/3rdparty
    ${CHAINBLOCKS_DIR}/deps/imgui_club/imgui_memory_editor
    ${CHAINBLOCKS_DIR}/deps/bgfx/examples/common/imgui
    ${CHAINBLOCKS_DIR}/deps/bimg/include
    ${CHAINBLOCKS_DIR}/deps/bgfx/3rdparty/dear-imgui
    ${CHAINBLOCKS_DIR}/deps/implot
    ${CHAINBLOCKS_DIR}/deps/bimg/3rdparty/astc-codec/include
    ${CHAINBLOCKS_DIR}/deps/miniaudio
    ${CHAINBLOCKS_DIR}/deps
    )

  if(WIN32)
    set(libsdlfile ${CMAKE_CURRENT_BINARY_DIR}/sdl_a/lib/libSDL2-static.a)
  elseif(X86_IOS_SIMULATOR)
    set(libsdlfile ${CMAKE_CURRENT_BINARY_DIR}/sdl_a/src/sdl_a/Xcode/SDL/build/Release-iphonesimulator/libSDL2.a)
    add_link_options(-F${CMAKE_CURRENT_BINARY_DIR}/sdl_a/src/sdl_a/Xcode/SDL/build/Release-iphonesimulator)
  elseif(IOS)
    set(libsdlfile ${CMAKE_CURRENT_BINARY_DIR}/sdl_a/src/sdl_a/Xcode/SDL/build/Release-iphoneos/libSDL2.a)
    add_link_options(-F${CMAKE_CURRENT_BINARY_DIR}/sdl_a/src/sdl_a/Xcode/SDL/build/Release-iphoneos)
  else()
    set(libsdlfile ${CMAKE_CURRENT_BINARY_DIR}/sdl_a/lib/libSDL2.a)
  endif()

  if(X86_IOS_SIMULATOR)
    ExternalProject_Add(sdl_a
      URL ${CHAINBLOCKS_DIR}/deps/SDL
      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/sdl_a
      CONFIGURE_COMMAND ""
      BUILD_IN_SOURCE true
      BUILD_COMMAND cd Xcode/SDL && xcodebuild -configuration Release -target "Static Library-iOS" -target hidapi-iOS -sdk iphonesimulator -arch x86_64
      INSTALL_COMMAND ""
      BUILD_BYPRODUCTS ${libsdlfile}
    )
  elseif(IOS)
    ExternalProject_Add(sdl_a
      URL ${CHAINBLOCKS_DIR}/deps/SDL
      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/sdl_a
      CONFIGURE_COMMAND ""
      BUILD_IN_SOURCE true
      BUILD_COMMAND cd Xcode/SDL && xcodebuild -configuration Release -target "Static Library-iOS" -target hidapi-iOS -sdk iphoneos -arch arm64
      INSTALL_COMMAND ""
      BUILD_BYPRODUCTS ${libsdlfile}
  )
  else()
    ExternalProject_Add(sdl_a
      URL ${CHAINBLOCKS_DIR}/deps/SDL
      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/sdl_a
      CMAKE_ARGS -DCMAKE_BUILD_TYPE=${EXTERNAL_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/sdl_a ${EXTRA_CMAKE_ARGS}
      BUILD_BYPRODUCTS ${libsdlfile}
    )
  endif()

  add_library(libsdl STATIC IMPORTED GLOBAL)
  set_target_properties(libsdl PROPERTIES IMPORTED_LOCATION ${libsdlfile})
  add_dependencies(libsdl sdl_a)

  if(WIN32)
    if(FORCE_USE_OPENGL)
      add_compile_definitions(BGFX_CONFIG_RENDERER_OPENGL=46)
      include_directories(${CHAINBLOCKS_DIR}/deps/bgfx/3rdparty/khronos)
    else()
      add_compile_definitions(BGFX_CONFIG_RENDERER_DIRECT3D11)
    endif()

    include_directories(
      ${CHAINBLOCKS_DIR}/deps/bx/include/compat/mingw
      ${CMAKE_CURRENT_BINARY_DIR}/sdl_a/include/SDL2
      )

    set(PLATFORM_SOURCES
      ${CHAINBLOCKS_DIR}/deps/bgfx/src/amalgamated.cpp
      ${CHAINBLOCKS_DIR}/src/extra/desktop.win.cpp
      )

    set(CB_PLATFORM_LIBS
      -lntdll -lOle32 -lImm32 -lWinmm -lGdi32 -lVersion -lOleAut32 -lSetupAPI -lPsapi -lD3D11 -lDXGI -lUserenv
      )

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      # 64 bits
      set(CB_PLATFORM_LIBS
        ${CB_PLATFORM_LIBS}
	      )
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
      # 32 bits
      set(CB_PLATFORM_LIBS
        ${CB_PLATFORM_LIBS}
	      )
    endif()
  elseif(APPLE)
    add_compile_definitions(BGFX_CONFIG_RENDERER_METAL SDL_VIDEO_OPENGL_EGL=0)

    include_directories(
      ${CHAINBLOCKS_DIR}/deps/bx/include/compat/osx
      ${CHAINBLOCKS_DIR}/deps/SDL/include
      ${CHAINBLOCKS_DIR}/deps/SDL/src/hidapi/hidapi
      )

    set(PLATFORM_SOURCES
      ${CHAINBLOCKS_DIR}/deps/bgfx/src/amalgamated.mm
      )

    if(IOS)
      enable_language(ASM)

      if(X86 OR X86_IOS_SIMULATOR)
        set(PLATFORM_SOURCES
          ${PLATFORM_SOURCES}
          ${CHAINBLOCKS_DIR}/deps/boost-context/src/posix/stack_traits.cpp
          ${CHAINBLOCKS_DIR}/deps/boost-context/src/asm/jump_x86_64_sysv_macho_gas.S
          ${CHAINBLOCKS_DIR}/deps/boost-context/src/asm/make_x86_64_sysv_macho_gas.S
          ${CHAINBLOCKS_DIR}/deps/boost-context/src/asm/ontop_x86_64_sysv_macho_gas.S
        )
      else()
        set(PLATFORM_SOURCES
          ${PLATFORM_SOURCES}
          ${CHAINBLOCKS_DIR}/deps/boost-context/src/posix/stack_traits.cpp
          ${CHAINBLOCKS_DIR}/deps/boost-context/src/asm/jump_arm64_aapcs_macho_gas.S
          ${CHAINBLOCKS_DIR}/deps/boost-context/src/asm/make_arm64_aapcs_macho_gas.S
          ${CHAINBLOCKS_DIR}/deps/boost-context/src/asm/ontop_arm64_aapcs_macho_gas.S
      )
      endif()
    endif()

    set(CB_PLATFORM_LIBS
      "-framework Foundation"
      "-framework CoreAudio"
      "-framework AudioToolbox"
      "-framework CoreVideo"
      "-framework IOKit"
      "-framework QuartzCore"
      "-framework Metal"
      "-framework Security"
      -liconv
      )

    if(IOS)
      set(CB_PLATFORM_LIBS
          ${CB_PLATFORM_LIBS}
          "-framework hidapi"
          "-framework AVFoundation"
          "-framework GameController"
          "-framework CoreMotion"
          "-framework CoreGraphics"
          "-framework CoreHaptics"
          "-framework UIKit")
    else()
       set(CB_PLATFORM_LIBS
          ${CB_PLATFORM_LIBS}
          "-framework Cocoa"
          "-framework Carbon"
          "-framework ForceFeedback")
    endif()
  elseif(LINUX)
    if(NOT EMSCRIPTEN)
      if(BGFX_OPENGL_VERSION)
        add_compile_definitions(BGFX_CONFIG_RENDERER_OPENGL_MIN_VERSION=${BGFX_OPENGL_VERSION})
      else()
        add_compile_definitions(BGFX_CONFIG_RENDERER_OPENGL_MIN_VERSION=43)
      endif()
    else()
      add_compile_definitions(BGFX_CONFIG_RENDERER_OPENGLES_MIN_VERSION=30)
    endif()

    include_directories(
      ${CHAINBLOCKS_DIR}/deps/bgfx/3rdparty/khronos
      ${CMAKE_CURRENT_BINARY_DIR}/sdl_a/include/SDL2
      )

    set(PLATFORM_SOURCES
      ${CHAINBLOCKS_DIR}/deps/bgfx/src/amalgamated.cpp
      )

    set(CB_PLATFORM_LIBS
      -lX11
      -lGL
      )
  endif()

  # kissfft
  ExternalProject_Add(kissfft
    URL ${CHAINBLOCKS_DIR}/deps/kissfft
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/kissfft-deps
    BUILD_IN_SOURCE True
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${EXTERNAL_BUILD_TYPE} ${EXTRA_CMAKE_ARGS} -DKISSFFT_ARCH_FLAGS=${ARCH_FLAGS} -DKISSFFT_PKGCONFIG=0 -DKISSFFT_STATIC=1 -DKISSFFT_TEST=0 -DKISSFFT_TOOLS=0
    BUILD_COMMAND cmake --build .
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/kissfft-deps/src/kissfft/libkissfft-float.a
  )

  add_library(libkissfft STATIC IMPORTED GLOBAL)
  set_target_properties(libkissfft PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/kissfft-deps/src/kissfft/libkissfft-float.a)
  add_dependencies(libkissfft kissfft)

  ExternalProject_Add(snappy_a
    GIT_REPOSITORY    https://github.com/chainblocks/snappy.git
    GIT_TAG           563e4e90f4ed6314a14055826f027b2239a8bf0e
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/snappy_a
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${EXTERNAL_BUILD_TYPE} -DSNAPPY_BUILD_TESTS=0 -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/snappy_a ${EXTRA_CMAKE_ARGS}
    BUILD_COMMAND cmake --build . --target snappy ${EXTRA_CMAKE_BUILD_ARGS}
    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/snappy_a/lib/libsnappy.a
    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/snappy_a/include/snappy.h
    )

  include_directories(${CMAKE_CURRENT_BINARY_DIR}/snappy_a/include)

  add_library(libsnappy STATIC IMPORTED GLOBAL)
  add_dependencies(libsnappy snappy_a)
  if(IOS)
    set_target_properties(libsnappy PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/snappy_a/src/snappy_a-build/${EXTRA_LIBS_DIR}/libsnappy.a)
  else()
    set_target_properties(libsnappy PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/snappy_a/lib/libsnappy.a)
  endif()

  set(CB_EXTRAS ${CB_EXTRAS} ${CHAINBLOCKS_DIR}/src/extra/snappy.cpp)
  set_source_files_properties(${CHAINBLOCKS_DIR}/src/extra/snappy.cpp PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/snappy_a/include/snappy.h)

  ExternalProject_Add(brotli_a
    GIT_REPOSITORY    https://github.com/chainblocks/brotli.git
    GIT_TAG           fcda9db7fd554ffb19c2410b9ada57cdabd19de5
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/brotli_a
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${EXTERNAL_BUILD_TYPE} -DBROTLI_BUNDLED_MODE=1 -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/brotli_a ${EXTRA_CMAKE_ARGS}
    BUILD_COMMAND cmake --build . --target brotlicommon-static brotlidec-static brotlienc-static ${EXTRA_CMAKE_BUILD_ARGS}
    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/brotli_a/src/brotli_a-build/libbrotlicommon-static.a
    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/brotli_a/src/brotli_a-build/libbrotlidec-static.a
    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/brotli_a/src/brotli_a-build/libbrotlienc-static.a
    BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/brotli_a/src/brotli_a/c/include/brotli/decode.h
    INSTALL_COMMAND ""
    )

  include_directories(${CMAKE_CURRENT_BINARY_DIR}/brotli_a/src/brotli_a/c/include)

  add_library(libbrotlicommon-static STATIC IMPORTED GLOBAL)
  add_dependencies(libbrotlicommon-static brotli_a)
  if(IOS)
    set_target_properties(libbrotlicommon-static PROPERTIES
      IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/brotli_a/src/brotli_a-build/${EXTRA_LIBS_DIR}/libbrotlicommon-static.a
      )
  else()
    set_target_properties(libbrotlicommon-static PROPERTIES
      IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/brotli_a/src/brotli_a-build/libbrotlicommon-static.a
      )
  endif()

  add_library(libbrotlidec-static STATIC IMPORTED GLOBAL)
  add_dependencies(libbrotlidec-static brotli_a)
  if(IOS)
    set_target_properties(libbrotlidec-static PROPERTIES
      IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/brotli_a/src/brotli_a-build/${EXTRA_LIBS_DIR}/libbrotlidec-static.a
      )
  else()
    set_target_properties(libbrotlidec-static PROPERTIES
      IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/brotli_a/src/brotli_a-build/libbrotlidec-static.a
      )
  endif()

  add_library(libbrotlienc-static STATIC IMPORTED GLOBAL)
  add_dependencies(libbrotlienc-static brotli_a)
  if(IOS)
    set_target_properties(libbrotlienc-static PROPERTIES
      IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/brotli_a/src/brotli_a-build/${EXTRA_LIBS_DIR}/libbrotlienc-static.a
      )
  else()
    set_target_properties(libbrotlienc-static PROPERTIES
      IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/brotli_a/src/brotli_a-build/libbrotlienc-static.a
      )
  endif()

  set(CB_EXTRAS ${CB_EXTRAS} ${CHAINBLOCKS_DIR}/src/extra/brotli.cpp)
  set_source_files_properties(${CHAINBLOCKS_DIR}/src/extra/brotli.cpp PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/brotli_a/src/brotli_a/c/include/brotli/decode.h)

  if(NOT IOS)
    set(CB_EXTRAS ${CB_EXTRAS} ${CHAINBLOCKS_DIR}/src/extra/audio.cpp)
  else()
    set(CB_EXTRAS ${CB_EXTRAS} ${CHAINBLOCKS_DIR}/src/extra/audio.mm)
  endif()

  set(CB_EXTRAS
    ${CB_EXTRAS}
    ${CHAINBLOCKS_DIR}/src/extra/runtime.cpp
    ${CHAINBLOCKS_DIR}/src/extra/bgfx.cpp
    ${CHAINBLOCKS_DIR}/src/extra/bgfx.shaderc.cpp
    ${CHAINBLOCKS_DIR}/src/extra/imgui.cpp
    ${CHAINBLOCKS_DIR}/src/extra/xr.cpp
    ${CHAINBLOCKS_DIR}/src/extra/dsp.cpp
    ${CHAINBLOCKS_DIR}/src/extra/gltf.cpp
    ${CHAINBLOCKS_DIR}/src/extra/inputs.cpp
    ${CHAINBLOCKS_DIR}/deps/bimg/src/image.cpp
    ${CHAINBLOCKS_DIR}/deps/bimg/src/image_gnf.cpp
    ${CHAINBLOCKS_DIR}/deps/bx/src/amalgamated.cpp
    ${CHAINBLOCKS_DIR}/deps/bgfx/3rdparty/dear-imgui/imgui.cpp
    ${CHAINBLOCKS_DIR}/deps/bgfx/3rdparty/dear-imgui/imgui_draw.cpp
    ${CHAINBLOCKS_DIR}/deps/bgfx/3rdparty/dear-imgui/imgui_widgets.cpp
    ${CHAINBLOCKS_DIR}/deps/bgfx/3rdparty/dear-imgui/imgui_tables.cpp
    ${CHAINBLOCKS_DIR}/deps/implot/implot.cpp
    ${CHAINBLOCKS_DIR}/deps/implot/implot_items.cpp

    ${PLATFORM_SOURCES}
    )

  # build rust blocks
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(EMSCRIPTEN)
      set(RUST_BUILD_SUBDIR wasm32-unknown-emscripten/debug)
      set(RUST_BUILD_OPTIONS ${RUST_BUILD_OPTIONS} --target=wasm32-unknown-emscripten)
    elseif(X86_IOS_SIMULATOR)
      set(RUST_BUILD_SUBDIR x86_64-apple-ios/debug)
      set(RUST_BUILD_OPTIONS ${RUST_BUILD_OPTIONS} --target=x86_64-apple-ios)
    elseif(IOS)
      set(RUST_BUILD_SUBDIR aarch64-apple-ios/debug)
      set(RUST_BUILD_OPTIONS ${RUST_BUILD_OPTIONS} --target=aarch64-apple-ios)
    elseif(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(RUST_BUILD_SUBDIR i686-pc-windows-gnu/debug)
      set(RUST_BUILD_OPTIONS ${RUST_BUILD_OPTIONS} --target=i686-pc-windows-gnu)
    else()
      set(RUST_BUILD_SUBDIR debug)
    endif()
  else()
    if(EMSCRIPTEN)
      set(RUST_BUILD_SUBDIR wasm32-unknown-emscripten/release)
      set(RUST_BUILD_OPTIONS ${RUST_BUILD_OPTIONS} --release --target=wasm32-unknown-emscripten)
    elseif(X86_IOS_SIMULATOR)
      set(RUST_BUILD_SUBDIR x86_64-apple-ios/release)
      set(RUST_BUILD_OPTIONS ${RUST_BUILD_OPTIONS} --release --target=x86_64-apple-ios)
    elseif(IOS)
      set(RUST_BUILD_SUBDIR aarch64-apple-ios/release)
      set(RUST_BUILD_OPTIONS ${RUST_BUILD_OPTIONS} --release --target=aarch64-apple-ios)
    elseif(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(RUST_BUILD_SUBDIR i686-pc-windows-gnu/release)
      set(RUST_BUILD_OPTIONS ${RUST_BUILD_OPTIONS} --release --target=i686-pc-windows-gnu)
    else()
      set(RUST_BUILD_SUBDIR release)
      set(RUST_BUILD_OPTIONS ${RUST_BUILD_OPTIONS} --release)
    endif()
  endif()

  if(NOT SKIP_RUST_BINDGEN)
    # So we got issues when building win32 and libclang...
    # let's bindgen on win64 only and the other OSs
    set(RUST_BINDGEN ,run_bindgen)
  endif()

  if(RUST_USE_LTO)
    set(RUST_LTO "-Clinker-plugin-lto -Clinker=clang -Clink-arg=-fuse-ld=lld ")
  endif()

  if(EMSCRIPTEN_PTHREADS)
    set(RUST_CRT "-Ctarget-feature=+atomics,+bulk-memory ")
    set(CARGO_Z "-Zbuild-std=panic_abort,std")
    set(CARGO_NIGHT "+nightly")
  endif()

  if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(RUSTFLAGS set RUSTFLAGS=${RUST_ARCH}${RUST_LTO}${RUST_CRT})
  else()
    set(RUSTFLAGS export RUSTFLAGS=${RUST_ARCH}${RUST_LTO}${RUST_CRT})
  endif()

  add_custom_target(
    rust_blocks
    COMMAND ${RUSTFLAGS}
    COMMAND cargo ${CARGO_NIGHT} build ${CARGO_Z} --features blocks${RUST_BINDGEN} ${RUST_BUILD_OPTIONS}
    WORKING_DIRECTORY ${CHAINBLOCKS_DIR}/rust
  )

  add_library(cb_rust_static STATIC IMPORTED GLOBAL)
  set_target_properties(cb_rust_static PROPERTIES IMPORTED_LOCATION ${CHAINBLOCKS_DIR}/target/${RUST_BUILD_SUBDIR}/libchainblocks.a)
  add_dependencies(cb_rust_static rust_blocks)
endif()

add_compile_definitions(REPLXX_STATIC)
set(replxx-source-patterns ${CHAINBLOCKS_DIR}/deps/replxx/src/*.cpp ${CHAINBLOCKS_DIR}/deps/replxx/src/*.cxx)
file(GLOB replxx-sources ${replxx-source-patterns})

set(RUNTIME_SRC
  ${replxx-sources}
  ${CHAINBLOCKS_DIR}/src/core/runtime.cpp
  ${CHAINBLOCKS_DIR}/src/core/ops_internal.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/assert.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/chains.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/logging.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/flow.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/seqs.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/casting.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/core.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/linalg.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/serialization.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/json.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/struct.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/time.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/strings.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/channels.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/random.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/imaging.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/bigint.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/fs.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/wasm.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/edn.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/reflection.cpp
  ${CHAINBLOCKS_DIR}/src/core/blocks/http.cpp
  ${CB_EXTRAS}
  )

if(NOT CB_NO_BIGINT_BLOCKS)
  set(
    RUNTIME_SRC
    ${RUNTIME_SRC}
    ${CHAINBLOCKS_DIR}/src/core/blocks/bigint.cpp)
else()
  add_compile_definitions(CB_NO_BIGINT_BLOCKS=1)
endif()

if(NOT EMSCRIPTEN)
  set(
    RUNTIME_SRC
    ${RUNTIME_SRC}
    ${CHAINBLOCKS_DIR}/src/core/blocks/process.cpp
    ${CHAINBLOCKS_DIR}/src/core/blocks/os.cpp
    ${CHAINBLOCKS_DIR}/src/core/blocks/network.cpp
    ${CHAINBLOCKS_DIR}/src/core/blocks/ws.cpp
    )

  if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_source_files_properties(${CHAINBLOCKS_DIR}/src/core/blocks/ws.cpp PROPERTIES COMPILE_FLAGS -O1)
    set_source_files_properties(${CHAINBLOCKS_DIR}/src/core/blocks/http.cpp PROPERTIES COMPILE_FLAGS -O1)
  endif()
else()
  set(
    RUNTIME_SRC
    ${RUNTIME_SRC}
    ${CHAINBLOCKS_DIR}/src/core/coro.cpp
    )
endif()

add_library(
  cb_static STATIC
  ${RUNTIME_SRC}
  )
set_property(TARGET cb_static PROPERTY COMPILE_FLAGS "-DCB_DLL_EXPORT")

set(CB_RUNTIME_LIBS cb_static)

# also shared lib target
# can't use OBJECT sadly due to xcode generator not liking it
add_library(
  cb_shared SHARED
  ${RUNTIME_SRC}
  )
set_property(TARGET cb_shared PROPERTY COMPILE_FLAGS "-DCB_DLL_EXPORT")
target_link_libraries(cb_shared ${BOOST_LIBS} ${EXTRA_LIBS} ${CB_PLATFORM_LIBS} ${EXTRA_SYS_LIBS})

add_dependencies(cb_static libwasm3)
add_dependencies(cb_shared libwasm3)
set(EXTRA_LIBS ${EXTRA_LIBS} libwasm3)

if(NOT BUILD_CORE_ONLY)
  add_dependencies(cb_static libkissfft)
  add_dependencies(cb_shared libkissfft)
  set(EXTRA_LIBS ${EXTRA_LIBS} libkissfft)

  add_dependencies(cb_static cb_rust_static)
  add_dependencies(cb_shared cb_rust_static)
  set(EXTRA_LIBS ${EXTRA_LIBS} cb_rust_static)

  if(NOT EMSCRIPTEN)
    # we link with emscripten SDL in the case of emscripten
    add_dependencies(cb_static libsdl)
    add_dependencies(cb_shared libsdl)
    set(EXTRA_LIBS ${EXTRA_LIBS} libsdl)
  endif()

  add_dependencies(cb_static libsnappy)
  add_dependencies(cb_shared libsnappy)
  set(EXTRA_LIBS ${EXTRA_LIBS} libsnappy)

  add_dependencies(cb_static libbrotlidec-static)
  add_dependencies(cb_shared libbrotlidec-static)
  set(EXTRA_LIBS ${EXTRA_LIBS} libbrotlidec-static)

  add_dependencies(cb_static libbrotlienc-static)
  add_dependencies(cb_shared libbrotlienc-static)
  set(EXTRA_LIBS ${EXTRA_LIBS} libbrotlienc-static)

  add_dependencies(cb_static libbrotlicommon-static)
  add_dependencies(cb_shared libbrotlicommon-static)
  set(EXTRA_LIBS ${EXTRA_LIBS} libbrotlicommon-static)
endif()

if(USE_LIBBACKTRACE)
  set(EXTRA_LIBS ${EXTRA_LIBS} ${CHAINBLOCKS_DIR}/deps/libbacktrace/build/lib/libbacktrace.a)
  add_compile_definitions(BOOST_STACKTRACE_USE_BACKTRACE)
  include_directories(${CHAINBLOCKS_DIR}/deps/libbacktrace/build/include)
endif()

### mal
add_executable(
  cbl
  "${CHAINBLOCKS_DIR}/src/mal/stepA_mal.cpp"
  "${CHAINBLOCKS_DIR}/src/mal/Core.cpp"
  "${CHAINBLOCKS_DIR}/src/mal/CBCore.cpp"
  "${CHAINBLOCKS_DIR}/src/mal/Environment.cpp"
  "${CHAINBLOCKS_DIR}/src/mal/Reader.cpp"
  "${CHAINBLOCKS_DIR}/src/mal/ReadLine.cpp"
  "${CHAINBLOCKS_DIR}/src/mal/String.cpp"
  "${CHAINBLOCKS_DIR}/src/mal/Types.cpp"
  "${CHAINBLOCKS_DIR}/src/mal/Validation.cpp"
  "${CHAINBLOCKS_DIR}/src/mal/CBCore.cpp"
  )
add_dependencies(cbl cb_static)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set_property(TARGET cbl PROPERTY COMPILE_FLAGS "-DCB_DLL_EXPORT")
target_link_libraries(cbl ${CB_RUNTIME_LIBS} ${EXTRA_LIBS} ${BOOST_LIBS} ${CB_PLATFORM_LIBS} ${EXTRA_SYS_LIBS})
if(EMSCRIPTEN)
  em_link_js_library(cbl ${CHAINBLOCKS_DIR}/src/core/blocks/core.js)
  if(NOT BUILD_CORE_ONLY)
    em_link_js_library(cbl ${CHAINBLOCKS_DIR}/src/extra/bgfx.shaderc.js)
  endif()
  if(EMSCRIPTEN_PTHREADS)
    set_target_properties(cbl PROPERTIES SUFFIX "-mt.js")
  else()
    set_target_properties(cbl PROPERTIES SUFFIX "-st.js")
  endif()
endif()
###

### our experimental edn eval
add_executable(
  cbedn
  "${CHAINBLOCKS_DIR}/src/core/edn/main.cpp"
  )
add_dependencies(cbedn cb_static)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set_property(TARGET cbedn PROPERTY COMPILE_FLAGS "-DCB_DLL_EXPORT")
target_link_libraries(cbedn ${CB_RUNTIME_LIBS} ${EXTRA_LIBS} ${BOOST_LIBS} ${CB_PLATFORM_LIBS} ${EXTRA_SYS_LIBS})
###

### TESTS

# catch2
ExternalProject_Add(catch2
  URL ${CHAINBLOCKS_DIR}/deps/Catch2
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/Catch2-deps
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/Catch2-deps ${EXTRA_CMAKE_ARGS}
  BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/Catch2-deps/lib/libCatch2.a
  BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/Catch2-deps/lib/libCatch2Main.a
)

add_library(libCatch2 STATIC IMPORTED GLOBAL)
set_target_properties(libCatch2 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/Catch2-deps/lib/libCatch2.a)
add_dependencies(libCatch2 catch2)

add_library(libCatch2Main STATIC IMPORTED GLOBAL)
set_target_properties(libCatch2Main PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/Catch2-deps/lib/libCatch2Main.a)
add_dependencies(libCatch2Main catch2)

# when building tests override some defines to enable internal (inside .cpp) tests
if(BUILD_INTERNAL_TESTS)
  set_source_files_properties(${CHAINBLOCKS_DIR}/src/extra/bgfx.cpp PROPERTIES COMPILE_FLAGS -DCB_INTERNAL_TESTS)
  set_source_files_properties(${CHAINBLOCKS_DIR}/src/extra/gltf.cpp PROPERTIES COMPILE_FLAGS -DCB_INTERNAL_TESTS)
endif()

add_executable(
  test_runtime
  "${CHAINBLOCKS_DIR}/src/tests/test_runtime.cpp"
  )
add_dependencies(test_runtime libCatch2)
add_dependencies(test_runtime libCatch2Main)
add_dependencies(test_runtime cb_static)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/Catch2-deps/include)
target_link_libraries(test_runtime ${CB_RUNTIME_LIBS} libCatch2Main libCatch2 ${EXTRA_LIBS} ${BOOST_LIBS} ${CB_PLATFORM_LIBS} ${EXTRA_SYS_LIBS})
# if(NOT WIN32 AND NOT EMSCRIPTEN AND NOT USE_VALGRIND)
#   target_compile_options(test_runtime PRIVATE -fsanitize=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer)
#   target_link_options(test_runtime PRIVATE -fsanitize=address)
# endif()
if(EMSCRIPTEN)
  em_link_js_library(test_runtime ${CHAINBLOCKS_DIR}/src/core/blocks/core.js)
  if(NOT BUILD_CORE_ONLY)
    em_link_js_library(test_runtime ${CHAINBLOCKS_DIR}/src/extra/bgfx.shaderc.js)
  endif()
endif()

add_executable(
  test_extra
  "${CHAINBLOCKS_DIR}/src/tests/test_extra.cpp"
  )
add_dependencies(test_extra libCatch2)
add_dependencies(test_extra libCatch2Main)
add_dependencies(test_extra cb_static)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/Catch2-deps/include)
target_link_libraries(test_extra ${CB_RUNTIME_LIBS} libCatch2Main libCatch2 ${EXTRA_LIBS} ${BOOST_LIBS} ${CB_PLATFORM_LIBS} ${EXTRA_SYS_LIBS})
# if(NOT WIN32 AND NOT EMSCRIPTEN AND NOT USE_VALGRIND)
#   target_compile_options(test_extra PRIVATE -fsanitize=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer)
#   target_link_options(test_extra PRIVATE -fsanitize=address)
# endif()
if(EMSCRIPTEN)
  em_link_js_library(test_extra ${CHAINBLOCKS_DIR}/src/core/blocks/core.js)
  if(NOT BUILD_CORE_ONLY)
    em_link_js_library(test_extra ${CHAINBLOCKS_DIR}/src/extra/bgfx.shaderc.js)
  endif()
endif()