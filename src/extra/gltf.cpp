#include "gltf/draw.cpp"
#include "gltf/load.cpp"

#define TINYGLTF_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

/*
TODO: GLTF.Simulate - depending on physics simulation blocks
TODO; GLTF.Animate - to play animations
*/
namespace chainblocks {
namespace gltf {
void registerBlocks() {
  REGISTER_CBLOCK("GLTF.Load", Load);
  REGISTER_CBLOCK("GLTF.Draw", Draw);
}
} // namespace gltf
} // namespace chainblocks

#ifdef CB_INTERNAL_TESTS
#include "gltf_tests.cpp"
#endif
