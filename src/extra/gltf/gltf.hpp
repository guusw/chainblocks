/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2021 Fragcolor Pte. Ltd. */

#pragma once
#include "blocks/shared.hpp"

#include <deque>
#include <filesystem>
#include <optional>
namespace fs = std::filesystem;
using LastWriteTime = decltype(fs::last_write_time(fs::path()));

#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_USE_CPP14
// #define TINYGLTF_ENABLE_DRACO
#include <tinygltf/tiny_gltf.h>
using GLTFModel = tinygltf::Model;
using GLTFImage = tinygltf::Image;
using namespace tinygltf;
#undef Model

namespace chainblocks {
namespace gltf {
struct GFXShader {
  GFXShader(bgfx::ProgramHandle handle, bgfx::ProgramHandle instanced)
      : handle(handle), handleInstanced(instanced) {}

  bgfx::ProgramHandle handle = BGFX_INVALID_HANDLE;
  bgfx::ProgramHandle handleInstanced = BGFX_INVALID_HANDLE;

  GFXShader(GFXShader &&other) {
    std::swap(handle, other.handle);
    std::swap(handleInstanced, other.handleInstanced);
  }

  ~GFXShader() {
    if (handle.idx != bgfx::kInvalidHandle) {
      bgfx::destroy(handle);
    }

    if (handleInstanced.idx != bgfx::kInvalidHandle) {
      bgfx::destroy(handleInstanced);
    }
  }
};

struct GFXTexture {
  uint16_t width;
  uint16_t height;
  bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

  GFXTexture(uint16_t width, uint16_t height) : width(width), height(height) {}

  GFXTexture(GFXTexture &&other) {
    std::swap(width, other.width);
    std::swap(height, other.height);
    std::swap(handle, other.handle);
  }

  ~GFXTexture() {
    if (handle.idx != bgfx::kInvalidHandle) {
      bgfx::destroy(handle);
    }
  }
};

struct GFXMaterial {
  std::string name;
  size_t hash;
  bool doubleSided{false};

  // textures and parameters
  std::shared_ptr<GFXTexture> baseColorTexture;
  std::array<float, 4> baseColor;

  std::shared_ptr<GFXTexture> metallicRoughnessTexture;
  float metallicFactor;
  float roughnessFactor;

  std::shared_ptr<GFXTexture> normalTexture;

  std::shared_ptr<GFXTexture> occlusionTexture;

  std::shared_ptr<GFXTexture> emissiveTexture;
  std::array<float, 3> emissiveColor;

  std::shared_ptr<GFXShader> shader;
};

using GFXMaterialRef = std::reference_wrapper<GFXMaterial>;

struct GFXPrimitive {
  bgfx::VertexBufferHandle vb = BGFX_INVALID_HANDLE;
  bgfx::IndexBufferHandle ib = BGFX_INVALID_HANDLE;
  bgfx::VertexLayout layout{};
  uint64_t stateFlags{0};
  std::optional<GFXMaterialRef> material;

  GFXPrimitive() {}

  GFXPrimitive(GFXPrimitive &&other) {
    std::swap(vb, other.vb);
    std::swap(ib, other.ib);
    std::swap(layout, other.layout);
    std::swap(material, other.material);
  }

  ~GFXPrimitive() {
    if (vb.idx != bgfx::kInvalidHandle) {
      bgfx::destroy(vb);
    }
    if (ib.idx != bgfx::kInvalidHandle) {
      bgfx::destroy(ib);
    }
  }
};

using GFXPrimitiveRef = std::reference_wrapper<GFXPrimitive>;

struct GFXMesh {
  std::string name;
  std::vector<GFXPrimitiveRef> primitives;
};

using GFXMeshRef = std::reference_wrapper<GFXMesh>;

struct Node;
using NodeRef = std::reference_wrapper<Node>;

struct GFXSkin {
  GFXSkin(const Skin &skin, const GLTFModel &gltf) {
    if (skin.skeleton != -1) {
      skeleton = skin.skeleton;
    }

    joints = skin.joints;

    if (skin.inverseBindMatrices != -1) {
      auto &mats_a = gltf.accessors[skin.inverseBindMatrices];
      assert(mats_a.type == TINYGLTF_TYPE_MAT4);
      assert(mats_a.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
      auto &view = gltf.bufferViews[mats_a.bufferView];
      auto &buffer = gltf.buffers[view.buffer];
      auto it = buffer.data.begin() + view.byteOffset + mats_a.byteOffset;
      const auto stride = mats_a.ByteStride(view);
      for (size_t i = 0; i < mats_a.count; i++) {
        bindPoses.emplace_back(Mat4::FromArrayUnsafe((float *)&(*it)));
        it += stride;
      }
    }
  }

  std::optional<int> skeleton;
  std::vector<int> joints;
  std::vector<Mat4> bindPoses;
};

using GFXSkinRef = std::reference_wrapper<GFXSkin>;

struct Node {
  std::string name;
  Mat4 transform;

  std::optional<GFXMeshRef> mesh;
  std::optional<GFXSkinRef> skin;

  std::vector<NodeRef> children;
};

struct GFXAnimation {
  GFXAnimation(const Animation &anim) {}
};

struct Model {
  std::deque<Node> nodes;
  std::deque<GFXPrimitive> gfxPrimitives;
  std::unordered_map<uint32_t, GFXMesh> gfxMeshes;
  std::unordered_map<uint32_t, GFXMaterial> gfxMaterials;
  std::unordered_map<uint32_t, GFXSkin> gfxSkins;
  std::vector<NodeRef> rootNodes;
  std::unordered_map<std::string_view, GFXAnimation> animations;

  static constexpr uint32_t CC = 'gltf';
  static inline chainblocks::Type Type{
      {CBType::Object, {.object = {.vendorId = CoreCC, .typeId = CC}}}};
  static inline chainblocks::Type VarType = chainblocks::Type::VariableOf(Type);
  static inline ObjectVar<Model> Var{"GLTF-Model", CoreCC, CC};
};

} // namespace gltf
} // namespace chainblocks
