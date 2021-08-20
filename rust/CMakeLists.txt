unset(Rust_FEATURES)

if(NOT SKIP_RUST_BINDGEN)
  # So we got issues when building win32 and libclang...
  # let's bindgen on win64 only and the other OSs
  ADD_RUST_FEATURE(Rust_FEATURES run_bindgen)
endif()

add_custom_target(
  cargo-chainblocks
  ${CMAKE_COMMAND} -E env
  RUSTFLAGS="${Rust_FLAGS}"
  cargo ${Rust_CARGO_TOOLCHAIN} build ${Rust_CARGO_UNSTABLE_FLAGS} --target ${Rust_CARGO_TARGET} --features ${Rust_FEATURES} ${Rust_CARGO_FLAGS}
  WORKING_DIRECTORY ${CHAINBLOCKS_DIR}/rust
  USES_TERMINAL
)

add_library(chainblocks-rust STATIC IMPORTED GLOBAL)
add_dependencies(chainblocks-rust cargo-chainblocks)
set_target_properties(chainblocks-rust PROPERTIES
  IMPORTED_LOCATION ${CHAINBLOCKS_DIR}/target/${Rust_BUILD_SUBDIR}/libchainblocks.a
)

if(WIN32)
  target_link_libraries(chainblocks-rust INTERFACE Userenv)
endif()