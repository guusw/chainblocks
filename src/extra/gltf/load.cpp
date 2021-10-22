/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2021 Fragcolor Pte. Ltd. */

#include "../bgfx.hpp"
#include "blocks/shared.hpp"
#include "linalg_shim.hpp"
#include "runtime.hpp"

#include <deque>
#include <filesystem>
#include <fstream>
#include <optional>

namespace fs = std::filesystem;
using LastWriteTime = decltype(fs::last_write_time(fs::path()));

#include <boost/algorithm/string.hpp>

#include <nlohmann/json.hpp>
#include <stb_image.h>
#include <stb_image_write.h>

#include "cache.hpp"
#include "gltf.hpp"

/*
TODO: GLTF.Simulate - depending on physics simulation blocks
TODO; GLTF.Animate - to play animations
*/
namespace chainblocks {
namespace gltf {

bool LoadImageData(GLTFImage *image, const int image_idx, std::string *err,
                   std::string *warn, int req_width, int req_height,
                   const unsigned char *bytes, int size, void *user_pointer) {
  return tinygltf::LoadImageData(image, image_idx, err, warn, req_width,
                                 req_height, bytes, size, user_pointer);
}

static inline std::string _shadersVarying;
static inline std::string _shadersVSEntry;
static inline std::string _shadersPSEntry;

struct Light {
  float position[4] = {0};
  float color[4] = {0};

  Light() {
    color[0] = 1.0f;
    color[1] = 1.0f;
    color[2] = 1.0f;
    color[3] = 1.0f;
  }

  float getRadius() const { return position[3]; }
  void setRadius(float radius) const { position[3] = radius; }
};

static void GetLights(Light *outLights, uint32_t maxNumLights,
                      uint32_t &numLights) {
  numLights = 0;
  if (numLights < maxNumLights) {
    Light &light = outLights[numLights];
    light.position[0] = -2.0f;
    light.position[1] = 1.0f;
    light.position++ numLights;
  }
}

struct PrimitiveContext {
  std::vector<
      std::pair<bgfx::Attrib::Enum, std::reference_wrapper<const Accessor>>>
      accessors;                                 // TODO
  GFXPrimitive prims{};                          // TODO
  std::unordered_set<std::string> shaderDefines; // TODO
  std::string varyings;                          // TODO
  uint32_t vertexSize = 0;                       // TODO
  size_t offsetSize = 0;                         // TODO

  GFXMesh &mesh;      // TODO
  CBContext *context; // TODO
  TextureCache &_texturesCache;
  ShadersCache &_shadersCache;
  Model *_model;
  IShaderCompiler *_shaderCompiler;
  const GLTFModel &_gltf;
  Node &node;                         // TODO
  const tinygltf::Primitive &glprims; // TODO
  const int maxBones;                 // TODO
  bool _withTextures{true};
  bool _withShaders{true};

  PrimitiveContext(GFXMesh &mesh, CBContext *context,
                   TextureCache &_texturesCache, ShadersCache &_shadersCache,
                   Model *model, IShaderCompiler *_shaderCompiler,
                   const GLTFModel &gltf, Node &node,
                   const tinygltf::Primitive &primitive, int maxBones,
                   bool _withTextures, bool _withShaders)
      : mesh(mesh), context(context), _texturesCache(_texturesCache),
        _shadersCache(_shadersCache), _model(model),
        _shaderCompiler(_shaderCompiler), _gltf(gltf), node(node),
        glprims(primitive), maxBones(maxBones), _withTextures(_withTextures),
        _withShaders(_withShaders) {
    varyings = _shadersVarying;
  }

  void addPositionAttribute(int attributeIdx) {
    accessors.emplace_back(bgfx::Attrib::Position,
                           _gltf.accessors[attributeIdx]);
    vertexSize += sizeof(float) * 3;
  }

  void addNormalAttribute(int attributeIdx) {
    accessors.emplace_back(bgfx::Attrib::Normal, _gltf.accessors[attributeIdx]);
    vertexSize += sizeof(float) * 3;
    shaderDefines.insert("CB_HAS_NORMAL");
    varyings.append("vec3 a_normal : NORMAL;\n");
  }

  void addTangentAttribute(int attributeIdx) {
    accessors.emplace_back(bgfx::Attrib::Tangent,
                           _gltf.accessors[attributeIdx]);
    shaderDefines.insert("CB_HAS_TANGENT");
    varyings.append("vec4 a_tangent : TANGENT;\n");
    vertexSize += sizeof(float) * 4;
  }

  void addTexcoordAttribute(int attributeIdx, int index) {
    if (index >= 4) {
      // 4 because we use an extra 4 for instanced
      // matrix... also we might use those for extra
      // bones!
      throw ActivationError("GLTF TEXCOORD_ limit exceeded.");
    }
    auto texcoord =
        decltype(bgfx::Attrib::TexCoord0)(int(bgfx::Attrib::TexCoord0) + index);
    accessors.emplace_back(texcoord, _gltf.accessors[attributeIdx]);
    vertexSize += sizeof(float) * 2;
    auto idxStr = std::to_string(index);
    shaderDefines.insert("CB_HAS_TEXCOORD_" + idxStr);
    varyings.append("vec2 a_texcoord" + idxStr + " : TEXCOORD" + idxStr +
                    ";\n");
  }

  void populateAccessors() {
    for (const auto &[attributeName, attributeIdx] : glprims.attributes) {
      if (attributeName == "POSITION") {
        addPositionAttribute(attributeIdx);
      } else if (attributeName == "NORMAL") {
        addNormalAttribute(attributeIdx);
      } else if (attributeName == "TANGENT") {
        addTangentAttribute(attributeIdx);
      } else if (boost::starts_with(attributeName, "TEXCOORD_")) {
        int strIndex = std::stoi(attributeName.substr(9));
        addTexcoordAttribute(attributeIdx, strIndex);

      } else if (boost::starts_with(attributeName, "COLOR_")) {
        int strIndex = std::stoi(attributeName.substr(6));
        if (strIndex >= 4) {
          throw ActivationError("GLTF COLOR_ limit exceeded.");
        }
        auto color = decltype(bgfx::Attrib::Color0)(int(bgfx::Attrib::Color0) +
                                                    strIndex);
        accessors.emplace_back(color, _gltf.accessors[attributeIdx]);
        vertexSize += 4;
        shaderDefines.insert("CB_HAS_" + attributeName);
        auto idxStr = std::to_string(strIndex);
        varyings.append("vec4 a_color" + idxStr + " : COLOR" + idxStr + ";\n");
      } else if (boost::starts_with(attributeName, "JOINTS_")) {
        int strIndex = std::stoi(attributeName.substr(7));

        shaderDefines.insert("CB_HAS_JOINTS_" + std::to_string(strIndex));
        if (strIndex == 0) {
          varyings.append("uvec4 a_indices : BLENDINDICES;\n");
          accessors.emplace_back(bgfx::Attrib::Indices,
                                 _gltf.accessors[attributeIdx]);
        } else if (strIndex == 1) {
          // TEXCOORD2
          varyings.append("uvec4 a_indices1 : TEXCOORD2;\n");
          accessors.emplace_back(bgfx::Attrib::Indices, // pass Indices for
                                                        // the next loop pass
                                 // but we will need to adjust this to
                                 // TexCoord2 in there
                                 _gltf.accessors[attributeIdx]);
        } else {
          throw ActivationError("Too many bones");
        }

        vertexSize += maxBones > 256 ? sizeof(uint16_t) * 4 : 4;
      } else if (boost::starts_with(attributeName, "WEIGHTS_")) {
        int strIndex = std::stoi(attributeName.substr(8));

        shaderDefines.insert("CB_HAS_WEIGHT_" + std::to_string(strIndex));
        if (strIndex == 0) {
          varyings.append("vec4 a_weight : BLENDWEIGHT;\n");
          accessors.emplace_back(bgfx::Attrib::Weight,
                                 _gltf.accessors[attributeIdx]);
        } else if (strIndex == 1) {
          // TEXCOORD3
          varyings.append("vec4 a_weight1 : TEXCOORD3;\n");
          accessors.emplace_back(bgfx::Attrib::TexCoord3,
                                 _gltf.accessors[attributeIdx]);
        } else {
          throw ActivationError("Too many bones");
        }

        vertexSize += sizeof(float) * 4;
      } else {
        CBLOG_WARNING("Ignored a primitive attribute: {}", attributeName);
      }
    }
  }

  void sortAccessors() {
    std::sort(accessors.begin(), accessors.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
  }

  GFXMaterial defaultMaterial(CBContext *context, const GLTFModel &_gltf,
                              std::unordered_set<std::string> &shaderDefines) {
    GFXMaterial material{"DEFAULT", std::hash<std::string_view>()("DEFAULT"),
                         false};

    Light lights[4];
    uint32_t numLights = 0;
    getLights(lights, numLights, numLights);

    if (numLights > 0) {
      shaderDefines.insert("CB_NUM_LIGHTS" + std::to_string(numLights));
    }

    if (_withShaders) {
      std::string shaderDefinesStr;
      for (const auto &define : shaderDefines) {
        shaderDefinesStr += define + ";";
      }
      std::string instancedVaryings = varyings;
      instancedVaryings.append("vec4 i_data0 : TEXCOORD7;\n");
      instancedVaryings.append("vec4 i_data1 : TEXCOORD6;\n");
      instancedVaryings.append("vec4 i_data2 : TEXCOORD5;\n");
      instancedVaryings.append("vec4 i_data3 : TEXCOORD4;\n");
      auto hash = ShadersCache::hashShader(shaderDefinesStr);
      auto shader = _shadersCache.find(hash);
      if (!shader) {
        // compile or fetch cached shaders
        // vertex
        bgfx::ShaderHandle vsh;
        {
          auto bytecode = _shaderCompiler->compile(
              varyings, _shadersVSEntry, "v", shaderDefinesStr, context);
          auto mem = bgfx::copy(bytecode.payload.bytesValue,
                                bytecode.payload.bytesSize);
          vsh = bgfx::createShader(mem);
        }
        // vertex - instanced
        bgfx::ShaderHandle ivsh;
        {
          auto bytecode = _shaderCompiler->compile(
              instancedVaryings, _shadersVSEntry, "v",
              shaderDefinesStr += "CB_INSTANCED;", context);
          auto mem = bgfx::copy(bytecode.payload.bytesValue,
                                bytecode.payload.bytesSize);
          ivsh = bgfx::createShader(mem);
        }
        // pixel
        bgfx::ShaderHandle psh;
        {
          auto bytecode = _shaderCompiler->compile(
              varyings, _shadersPSEntry, "f", shaderDefinesStr, context);
          auto mem = bgfx::copy(bytecode.payload.bytesValue,
                                bytecode.payload.bytesSize);
          psh = bgfx::createShader(mem);
        }
        shader =
            std::make_shared<GFXShader>(bgfx::createProgram(vsh, psh, true),
                                        bgfx::createProgram(ivsh, psh, true));
        _shadersCache.add(hash, shader);
      }
      material.shader = shader;
    }

    return material;
  }

  std::shared_ptr<GFXTexture> getOrLoadTexture(const GLTFModel &_gltf,
                                               const Texture &gltexture) {
    if (_withTextures && gltexture.source != -1) {
      const auto &image = _gltf.images[gltexture.source];
      const auto imgHash = TextureCache::hashTexture(gltexture, image);
      auto texture = _texturesCache.find(imgHash);
      if (!texture) {
        texture = std::make_shared<GFXTexture>(uint16_t(image.width),
                                               uint16_t(image.height));

        BGFX_TEXTURE2D_CREATE(image.bits, image.component, texture);

        if (texture->handle.idx == bgfx::kInvalidHandle)
          throw ActivationError("Failed to create texture");

        auto mem = bgfx::copy(image.image.data(), image.image.size());
        bgfx::updateTexture2D(texture->handle, 0, 0, 0, 0, texture->width,
                              texture->height, mem);

        _texturesCache.add(imgHash, texture);
      }
      // we are sure we added the texture but still return it from the table
      // as there might be some remote chance of racing and both threads
      // loading the same in such case we rather have one destroyed
      return _texturesCache.find(imgHash);
    } else {
      return nullptr;
    }
  }

  GFXMaterial processMaterial(CBContext *context, const GLTFModel &_gltf,
                              const Material &glmaterial,
                              std::unordered_set<std::string> &shaderDefines) {
    GFXMaterial material{glmaterial.name,
                         std::hash<std::string_view>()(glmaterial.name),
                         glmaterial.doubleSided};

    // do an extension check
    for (const auto &[name, val] : glmaterial.extensions) {
      if (name == "KHR_materials_unlit") {
        shaderDefines.insert("CB_UNLIT");
      }
    }

    if (_numLights > 0) {
      shaderDefines.insert("CB_NUM_LIGHTS" + std::to_string(_numLights));
    }

    if (glmaterial.pbrMetallicRoughness.baseColorTexture.index != -1) {
      material.baseColorTexture =
          getOrLoadTexture(_gltf, _gltf.textures[glmaterial.pbrMetallicRoughness
                                                     .baseColorTexture.index]);
      shaderDefines.insert("CB_PBR_COLOR_TEXTURE");
    }
    const auto colorFactorSize =
        glmaterial.pbrMetallicRoughness.baseColorFactor.size();
    if (colorFactorSize > 0) {
      for (size_t i = 0; i < colorFactorSize && i < 4; i++) {
        material.baseColor[i] =
            glmaterial.pbrMetallicRoughness.baseColorFactor[i];
      }
      shaderDefines.insert("CB_PBR_COLOR_FACTOR");
    }

    material.metallicFactor = glmaterial.pbrMetallicRoughness.metallicFactor;
    material.roughnessFactor = glmaterial.pbrMetallicRoughness.roughnessFactor;
    if (glmaterial.pbrMetallicRoughness.metallicRoughnessTexture.index != -1) {
      material.metallicRoughnessTexture = getOrLoadTexture(
          _gltf, _gltf.textures[glmaterial.pbrMetallicRoughness
                                    .metallicRoughnessTexture.index]);
      shaderDefines.insert("CB_PBR_METALLIC_ROUGHNESS_TEXTURE");
    }

    if (glmaterial.normalTexture.index != -1) {
      material.normalTexture = getOrLoadTexture(
          _gltf, _gltf.textures[glmaterial.normalTexture.index]);
      shaderDefines.insert("CB_NORMAL_TEXTURE");
    }

    if (_withShaders) {
      std::string shaderDefinesStr;
      for (const auto &define : shaderDefines) {
        shaderDefinesStr += define + ";";
      }
      std::string instancedVaryings = varyings;
      instancedVaryings.append("vec4 i_data0 : TEXCOORD7;\n");
      instancedVaryings.append("vec4 i_data1 : TEXCOORD6;\n");
      instancedVaryings.append("vec4 i_data2 : TEXCOORD5;\n");
      instancedVaryings.append("vec4 i_data3 : TEXCOORD4;\n");
      auto hash = ShadersCache::hashShader(shaderDefinesStr);
      auto shader = _shadersCache.find(hash);
      if (!shader) {
        // compile or fetch cached shaders
        // vertex
        bgfx::ShaderHandle vsh;
        {
          auto bytecode = _shaderCompiler->compile(
              varyings, _shadersVSEntry, "v", shaderDefinesStr, context);
          auto mem = bgfx::copy(bytecode.payload.bytesValue,
                                bytecode.payload.bytesSize);
          vsh = bgfx::createShader(mem);
        }
        // vertex - instanced
        bgfx::ShaderHandle ivsh;
        {
          auto bytecode = _shaderCompiler->compile(
              instancedVaryings, _shadersVSEntry, "v",
              shaderDefinesStr += "CB_INSTANCED;", context);
          auto mem = bgfx::copy(bytecode.payload.bytesValue,
                                bytecode.payload.bytesSize);
          ivsh = bgfx::createShader(mem);
        }
        // pixel
        bgfx::ShaderHandle psh;
        {
          auto bytecode = _shaderCompiler->compile(
              varyings, _shadersPSEntry, "f", shaderDefinesStr, context);
          auto mem = bgfx::copy(bytecode.payload.bytesValue,
                                bytecode.payload.bytesSize);
          psh = bgfx::createShader(mem);
        }
        shader =
            std::make_shared<GFXShader>(bgfx::createProgram(vsh, psh, true),
                                        bgfx::createProgram(ivsh, psh, true));
        _shadersCache.add(hash, shader);
      }
      material.shader = shader;
    }

    return material;
  }

  void createAndFillVertexBuffer() {
    const auto &firstAccessor = accessors[0].second;
    const auto vertexCount = firstAccessor.get().count;
    const auto totalSize = uint32_t(vertexCount) * vertexSize;
    auto vbuffer = bgfx::alloc(totalSize);

    prims.layout.begin();
    int boneSet = 0;

    for (const auto &[attrib, accessorRef] : accessors) {
      const auto &accessor = accessorRef.get();
      const auto &view = _gltf.bufferViews[accessor.bufferView];
      const auto &buffer = _gltf.buffers[view.buffer];
      const auto dataBeg =
          buffer.data.begin() + view.byteOffset + accessor.byteOffset;
      const auto stride = accessor.ByteStride(view);
      switch (attrib) {
      case bgfx::Attrib::Position:
      case bgfx::Attrib::Normal: {
        const auto size = sizeof(float) * 3;
        auto vbufferOffset = offsetSize;
        offsetSize += size;

        if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
            accessor.type != TINYGLTF_TYPE_VEC3) {
          throw ActivationError("Position/Normal vector data was not "
                                "a float32 vector of 3");
        }

        size_t idx = 0;
        auto it = dataBeg;
        while (idx < vertexCount) {
          const uint8_t *chunk = &(*it);
          memcpy(vbuffer->data + vbufferOffset, chunk, size);

          vbufferOffset += vertexSize;
          it += stride;
          idx++;
        }

        // also update layout
        prims.layout.add(attrib, 3, bgfx::AttribType::Float);
      } break;
        break;
      case bgfx::Attrib::Tangent: {
        // w is handedness
        const auto size = sizeof(float) * 4;
        auto vbufferOffset = offsetSize;
        offsetSize += size;

        if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
            accessor.type != TINYGLTF_TYPE_VEC4) {
          throw ActivationError("Tangent vector data was not a "
                                "float32 vector of 4");
        }

        size_t idx = 0;
        auto it = dataBeg;
        while (idx < vertexCount) {
          const float *chunk = (float *)&(*it);
          memcpy(vbuffer->data + vbufferOffset, chunk, size);

          vbufferOffset += vertexSize;
          it += stride;
          idx++;
        }

        // also update layout
        prims.layout.add(attrib, 4, bgfx::AttribType::Float);
      } break;
      case bgfx::Attrib::Color0:
      case bgfx::Attrib::Color1:
      case bgfx::Attrib::Color2:
      case bgfx::Attrib::Color3: {
        const auto elemSize = [&]() {
          if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
            return 4;
          else if (accessor.componentType ==
                   TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            return 2;
          else // BYTE
            return 1;
        }();
        const auto elemNum = [&]() {
          if (accessor.type == TINYGLTF_TYPE_VEC3)
            return 3;
          else // VEC4
            return 4;
        }();
        const auto osize = 4;
        auto vbufferOffset = offsetSize;
        offsetSize += osize;

        size_t idx = 0;
        auto it = dataBeg;
        while (idx < vertexCount) {
          switch (elemNum) {
          case 3: {
            switch (elemSize) {
            case 4: { // float
              const float *chunk = (float *)&(*it);
              uint8_t *buf = vbuffer->data + vbufferOffset;
              buf[0] = uint8_t(chunk[0] * 255);
              buf[1] = uint8_t(chunk[1] * 255);
              buf[2] = uint8_t(chunk[2] * 255);
              buf[3] = 255;
            } break;
            case 2: {
              const uint16_t *chunk = (uint16_t *)&(*it);
              uint8_t *buf = vbuffer->data + vbufferOffset;
              buf[0] = uint8_t((float(chunk[0]) / float(UINT16_MAX)) * 255);
              buf[1] = uint8_t((float(chunk[1]) / float(UINT16_MAX)) * 255);
              buf[2] = uint8_t((float(chunk[2]) / float(UINT16_MAX)) * 255);
              buf[3] = 255;
            } break;
            case 1: {
              const uint8_t *chunk = (uint8_t *)&(*it);
              uint8_t *buf = vbuffer->data + vbufferOffset;
              memcpy(buf, chunk, 3);
              buf[3] = 255;
            } break;
            default:
              CBLOG_FATAL("invalid state");
              break;
            }
          } break;
          case 4: {
            switch (elemSize) {
            case 4: { // float
              const float *chunk = (float *)&(*it);
              uint8_t *buf = vbuffer->data + vbufferOffset;
              buf[0] = uint8_t(chunk[0] * 255);
              buf[1] = uint8_t(chunk[1] * 255);
              buf[2] = uint8_t(chunk[2] * 255);
              buf[3] = uint8_t(chunk[3] * 255);
            } break;
            case 2: {
              const uint16_t *chunk = (uint16_t *)&(*it);
              uint8_t *buf = vbuffer->data + vbufferOffset;
              buf[0] = uint8_t((float(chunk[0]) / float(UINT16_MAX)) * 255);
              buf[1] = uint8_t((float(chunk[1]) / float(UINT16_MAX)) * 255);
              buf[2] = uint8_t((float(chunk[2]) / float(UINT16_MAX)) * 255);
              buf[3] = uint8_t((float(chunk[3]) / float(UINT16_MAX)) * 255);
            } break;
            case 1: {
              const uint8_t *chunk = (uint8_t *)&(*it);
              uint8_t *buf = vbuffer->data + vbufferOffset;
              memcpy(buf, chunk, 4);
            } break;
            default:
              CBLOG_FATAL("invalid state");
              break;
            }
          } break;
          default:
            CBLOG_FATAL("invalid state");
            break;
          }

          vbufferOffset += vertexSize;
          it += stride;
          idx++;
        }

        prims.layout.add(attrib, 4, bgfx::AttribType::Uint8, true);
      } break;
      case bgfx::Attrib::TexCoord0:
      case bgfx::Attrib::TexCoord1:
      case bgfx::Attrib::TexCoord2:
      case bgfx::Attrib::TexCoord3:
      case bgfx::Attrib::TexCoord4:
      case bgfx::Attrib::TexCoord5:
      case bgfx::Attrib::TexCoord6:
      case bgfx::Attrib::TexCoord7:
      case bgfx::Attrib::Weight: {
        const auto elemSize = [&]() {
          if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
            return 4;
          else if (accessor.componentType ==
                   TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            return 2;
          else if (accessor.componentType ==
                   TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            return 1;
          else
            throw ActivationError("TexCoord/Weight accessor invalid size");
        }();

        const auto vecSize = [&]() {
          if (accessor.type == TINYGLTF_TYPE_VEC4)
            return 4;
          else if (accessor.type == TINYGLTF_TYPE_VEC3)
            return 3;
          else if (accessor.type == TINYGLTF_TYPE_VEC2)
            return 2;
          else
            throw ActivationError("TexCoord/Weight accessor invalid type");
        }();

        const auto osize = sizeof(float) * vecSize;
        auto vbufferOffset = offsetSize;
        offsetSize += osize;

        size_t idx = 0;
        auto it = dataBeg;
        while (idx < vertexCount) {
          switch (elemSize) {
          case 4: { // float
            const float *chunk = (float *)&(*it);
            float *buf = (float *)(vbuffer->data + vbufferOffset);
            for (auto i = 0; i < vecSize; i++) {
              buf[i] = chunk[i];
            }
          } break;
          case 2: {
            const uint16_t *chunk = (uint16_t *)&(*it);
            float *buf = (float *)(vbuffer->data + vbufferOffset);
            for (auto i = 0; i < vecSize; i++) {
              buf[i] = float(chunk[i]) / float(UINT16_MAX);
            }
          } break;
          case 1: {
            const uint8_t *chunk = (uint8_t *)&(*it);
            float *buf = (float *)(vbuffer->data + vbufferOffset);
            for (auto i = 0; i < vecSize; i++) {
              buf[i] = float(chunk[i]) / float(UINT8_MAX);
            }
          } break;
          default:
            CBLOG_FATAL("invalid state");
            break;
          }

          vbufferOffset += vertexSize;
          it += stride;
          idx++;
        }

        prims.layout.add(attrib, vecSize, bgfx::AttribType::Float);
      } break;
      case bgfx::Attrib::Indices: {
        size_t elemSize =
            accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT
                ? 2
                : 1;

        const auto size = maxBones > 256 ? sizeof(uint16_t) * 4 : 4;
        auto vbufferOffset = offsetSize;
        offsetSize += size;

        if (accessor.type != TINYGLTF_TYPE_VEC4) {
          throw ActivationError("Joints vector data was "
                                "not a  vector of 4");
        }

        size_t idx = 0;
        auto it = dataBeg;
        while (idx < vertexCount) {
          if (maxBones > 256) {
            switch (elemSize) {
            case 2: { // uint16_t
              const uint16_t *chunk = (uint16_t *)&(*it);
              uint16_t *buf = (uint16_t *)(vbuffer->data + vbufferOffset);
              buf[0] = chunk[0];
              buf[1] = chunk[1];
              buf[2] = chunk[2];
              buf[3] = chunk[3];
            } break;
            case 1: { // uint8_t
              const uint8_t *chunk = (uint8_t *)&(*it);
              uint16_t *buf = (uint16_t *)(vbuffer->data + vbufferOffset);
              buf[0] = uint16_t(chunk[0]);
              buf[1] = uint16_t(chunk[1]);
              buf[2] = uint16_t(chunk[2]);
              buf[3] = uint16_t(chunk[3]);
            } break;
            default:
              CBLOG_FATAL("invalid state");
              break;
            }
          } else {
            switch (elemSize) {
            case 2: { // uint16_t
              const uint16_t *chunk = (uint16_t *)&(*it);
              uint8_t *buf = (uint8_t *)(vbuffer->data + vbufferOffset);
              buf[0] = uint8_t(chunk[0]);
              buf[1] = uint8_t(chunk[1]);
              buf[2] = uint8_t(chunk[2]);
              buf[3] = uint8_t(chunk[3]);
            } break;
            case 1: { // uint8_t
              const uint8_t *chunk = (uint8_t *)&(*it);
              uint8_t *buf = (uint8_t *)(vbuffer->data + vbufferOffset);
              buf[0] = chunk[0];
              buf[1] = chunk[1];
              buf[2] = chunk[2];
              buf[3] = chunk[3];
            } break;
            default:
              CBLOG_FATAL("invalid state");
              break;
            }
          }

          vbufferOffset += vertexSize;
          it += stride;
          idx++;
        }

        prims.layout.add(
            boneSet > 0 ? bgfx::Attrib::TexCoord2 : bgfx::Attrib::Indices, 4,
            maxBones > 256 ? bgfx::AttribType::Int16 : bgfx::AttribType::Uint8);
        boneSet++;
      } break;
      default:
        throw std::runtime_error("Invalid attribute.");
        break;
      }
    }

    // wrap up layout
    prims.layout.end();
    assert(prims.layout.getSize(1) == vertexSize);
    prims.vb = bgfx::createVertexBuffer(vbuffer, prims.layout);
  }

  void createAndFillIndexBuffer() {
    const auto &indices = _gltf.accessors[glprims.indices];
    const auto count = indices.count;
    int esize;
    if (count < UINT16_MAX) {
      esize = 2;
    } else {
      esize = 4;
    }
    auto ibuffer = bgfx::alloc(esize * count);
    auto offset = 0;
    int gsize;
    // https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#indices
    if (indices.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
      gsize = 4;
    } else if (indices.componentType ==
               TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
      gsize = 2;
    } else {
      gsize = 1;
    }
    const auto &view = _gltf.bufferViews[indices.bufferView];
    const auto &buffer = _gltf.buffers[view.buffer];
    const auto dataBeg =
        buffer.data.begin() + view.byteOffset + indices.byteOffset;
    const auto stride = indices.ByteStride(view);

#define FILL_INDEX                                                             \
  if (esize == 4) {                                                            \
    uint32_t *ibuf = (uint32_t *)(ibuffer->data + offset);                     \
    *ibuf = uint32_t(*chunk);                                                  \
  } else {                                                                     \
    uint16_t *ibuf = (uint16_t *)(ibuffer->data + offset);                     \
    *ibuf = uint16_t(*chunk);                                                  \
  }
    size_t idx = 0;
    auto it = dataBeg;
    while (idx < count) {
      switch (gsize) {
      case 4: {
        const uint32_t *chunk = (uint32_t *)&(*it);
        FILL_INDEX;
      } break;
      case 2: {
        const uint16_t *chunk = (uint16_t *)&(*it);
        FILL_INDEX;
      } break;
      case 1: {
        const uint8_t *chunk = (uint8_t *)&(*it);
        FILL_INDEX;
      } break;
      default:
        CBLOG_FATAL("invalid state");
        ;
        break;
      }

      offset += esize;
      it += stride;
      idx++;
    }
#undef FILL_INDEX
    prims.ib =
        bgfx::createIndexBuffer(ibuffer, esize == 4 ? BGFX_BUFFER_INDEX32 : 0);
  }

  void process() {
    // this is not used by us, but it's required by the
    // shader
    shaderDefines.insert("BGFX_CONFIG_MAX_BONES=1");
    shaderDefines.insert("CB_MAX_BONES=" + std::to_string(maxBones));
    // we gotta do few things here
    // build a layout
    // populate vb and ib

    populateAccessors();
    sortAccessors();

    if (accessors.size() > 0) {
      createAndFillVertexBuffer();

      // check if we have indices
      if (glprims.indices != -1) {
        createAndFillIndexBuffer();
      } else {
        prims.stateFlags = BGFX_STATE_PT_TRISTRIP;
      }

      const auto determinant = linalg::determinant(node.transform);
      if (!(determinant < 0.0)) {
        prims.stateFlags |= BGFX_STATE_FRONT_CCW;
      }

      if (glprims.material != -1) {
        auto it = _model->gfxMaterials.find(glprims.material);
        if (it != _model->gfxMaterials.end()) {
          prims.material = it->second;
        } else {
          prims.material =
              _model->gfxMaterials
                  .emplace(glprims.material,
                           processMaterial(context, _gltf,
                                           _gltf.materials[glprims.material],
                                           shaderDefines))
                  .first->second;
        }
      } else {
        // add default material here!
        auto it = _model->gfxMaterials.find(glprims.material);
        if (it != _model->gfxMaterials.end()) {
          prims.material = it->second;
        } else {
          prims.material =
              _model->gfxMaterials
                  .emplace(glprims.material,
                           defaultMaterial(context, _gltf, shaderDefines))
                  .first->second;
        }
      }

      mesh.primitives.emplace_back(
          _model->gfxPrimitives.emplace_back(std::move(prims)));
    }
  }
};

struct Load : public BGFX::BaseConsumer {
  static CBTypesInfo inputTypes() { return CoreInfo::StringType; }
  static CBTypesInfo outputTypes() { return Model::Type; }

  static inline Parameters Params{
      {"Textures",
       CBCCSTR("If the textures linked to this model should be loaded."),
       {CoreInfo::BoolType}},
      {"Shaders",
       CBCCSTR("If the shaders linked to this model should be compiled and/or "
               "loaded."),
       {CoreInfo::BoolType}},
      {"TransformBefore",
       CBCCSTR("An optional global transformation matrix to apply before the "
               "model root nodes transformations."),
       {CoreInfo::Float4x4Type, CoreInfo::NoneType}},
      {"TransformAfter",
       CBCCSTR("An optional global transformation matrix to apply after the "
               "model root nodes transformations."),
       {CoreInfo::Float4x4Type, CoreInfo::NoneType}},
      {"Animations", // TODO this should be a Set...
       CBCCSTR("A list of animations to load and prepare from the gltf file, "
               "if not found an error is raised."),
       {CoreInfo::StringSeqType, CoreInfo::NoneType}}};
  static CBParametersInfo parameters() { return Params; }

  GLTFModel _gltf;
  TinyGLTF _loader;
  Model *_model{nullptr};
  bool _withTextures{true};
  bool _withShaders{true};
  OwnedVar _animations;
  uint32_t _numLights{0};
  std::unique_ptr<IShaderCompiler> _shaderCompiler;
  std::optional<Mat4> _before;
  std::optional<Mat4> _after;

  void setup() {}

  CBTypeInfo compose(const CBInstanceData &data) {
    // Skip base compose, we don't care about that check
    // BGFX::BaseConsumer::compose(data);
    return Model::Type;
  }

  Shared<TextureCache> _texturesCache;
  Shared<ShadersCache> _shadersCache;

  std::mutex _shadersMutex;

  void setParam(int index, const CBVar &value) {
    switch (index) {
    case 0:
      _withTextures = value.payload.boolValue;
      break;
    case 1:
      _withShaders = value.payload.boolValue;
      break;
    case 2:
      if (value.valueType == CBType::None) {
        _before.reset();
      } else {
        // navigating c++ operators for Mat4
        Mat4 m;
        m = value;
        _before = m;
      }
      break;
    case 3:
      if (value.valueType == CBType::None) {
        _after.reset();
      } else {
        // navigating c++ operators for Mat4
        Mat4 m;
        m = value;
        _after = m;
      }
      break;
    case 4:
      _animations = value;
      break;
    default:
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
      return Var(_withTextures);
    case 1:
      return Var(_withShaders);
    case 2:
      if (_before)
        return *_before;
      else
        return Var::Empty;
    case 3:
      if (_after)
        return *_after;
      else
        return Var::Empty;
    case 4:
      return _animations;
    default:
      throw InvalidParameterIndex();
    }
  }

  void cleanup() {
    if (_model) {
      Model::Var.Release(_model);
      _model = nullptr;
    }

    if (_shaderCompiler) {
      _shaderCompiler = nullptr;
    }
  }

  void warmup(CBContext *ctx) {
    _shaderCompiler = makeShaderCompiler();
    _shaderCompiler->warmup(ctx);

    // lazily fill out varying and entry point shaders
    if (_shadersVarying.size() == 0) {
      std::unique_lock lock(_shadersMutex);
      // check again after unlock
      if (_shadersVarying.size() == 0) {
        auto failed = false;
        {
          std::ifstream stream("shaders/lib/gltf/varying.txt");
          if (!stream.is_open())
            failed = true;
          std::stringstream buffer;
          buffer << stream.rdbuf();
          _shadersVarying.assign(buffer.str());
        }
        {
          std::ifstream stream("shaders/lib/gltf/vs_entry.h");
          if (!stream.is_open())
            failed = true;
          std::stringstream buffer;
          buffer << stream.rdbuf();
          _shadersVSEntry.assign(buffer.str());
        }
        {
          std::ifstream stream("shaders/lib/gltf/ps_entry.h");
          if (!stream.is_open())
            failed = true;
          std::stringstream buffer;
          buffer << stream.rdbuf();
          _shadersPSEntry.assign(buffer.str());
        }
        if (failed) {
          CBLOG_FATAL("shaders library is missing");
        }
      }
    }
  }

  std::atomic_bool _canceled = false;
  CBVar activate(CBContext *context, const CBVar &input) {
    return awaitne(
        context, [&]() { return loadModel(context, input); },
        [&] { _canceled = true; });
  }

  void loadFromFile(const char *filename) {
    std::string err;
    std::string warn;
    bool success = false;
    fs::path filepath(filename);
    const auto &ext = filepath.extension();

    if (!fs::exists(filepath)) {
      throw ActivationError("GLTF model file does not exist.");
    }

    if (ext == ".glb") {
      success = _loader.LoadBinaryFromFile(&_gltf, &err, &warn, filename);
    } else {
      success = _loader.LoadASCIIFromFile(&_gltf, &err, &warn, filename);
    }

    if (!warn.empty()) {
      CBLOG_WARNING("GLTF warning: {}", warn);
    }
    if (!err.empty()) {
      CBLOG_ERROR("GLTF error: {}", err);
    }
    if (!success) {
      throw ActivationError("Failed to parse GLTF.");
    }
  }

  void processNodeTransform(Node &node, const tinygltf::Node &glnode) {
    if (glnode.matrix.size() != 0) {
      node.transform = Mat4::FromVector(glnode.matrix);
    } else {
      const auto t = linalg::translation_matrix(
          glnode.translation.size() != 0 ? Vec3::FromVector(glnode.translation)
                                         : Vec3());
      const auto r = linalg::rotation_matrix(
          glnode.rotation.size() != 0 ? Vec4::FromVector(glnode.rotation)
                                      : Vec4::Quaternion());
      const auto s = linalg::scaling_matrix(glnode.scale.size() != 0
                                                ? Vec3::FromVector(glnode.scale)
                                                : Vec3(1.0, 1.0, 1.0));
      node.transform = linalg::mul(linalg::mul(t, r), s);
    }

    if (_before) {
      node.transform = linalg::mul(*_before, node.transform);
    }

    if (_after) {
      node.transform = linalg::mul(node.transform, *_after);
    }
  }

  void processNodeMesh(CBContext *context, Node &node,
                       const tinygltf::Node &glnode) {
    auto it = _model->gfxMeshes.find(glnode.mesh);
    if (it != _model->gfxMeshes.end()) {
      node.mesh = it->second;
    } else {
      node.mesh = _model->gfxMeshes
                      .emplace(glnode.mesh,
                               std::move(createNodeMesh(context, node, glnode)))
                      .first->second;
    }
  }

  GFXMesh createNodeMesh(CBContext *context, Node &node,
                         const tinygltf::Node &glnode) {
    const auto &glmesh = _gltf.meshes[glnode.mesh];
    GFXMesh mesh{glmesh.name};
    for (const auto &glprim : glmesh.primitives) {
      PrimitiveContext primContext(mesh, context, _texturesCache(),
                                   _shadersCache(), _model,
                                   _shaderCompiler.get(), _gltf, node, glprim,
                                   maxBones, _withTextures, _withShaders);
      primContext.process();
    }

    return mesh;
  }

  NodeRef processNode(CBContext *context, const tinygltf::Node &glnode) {
    Node node;
    node.name = glnode.name;

    processNodeTransform(node, glnode);

    if (glnode.mesh != -1) {
      processNodeMesh(context, node, glnode);
    }

    if (glnode.skin != -1) {
      auto it = _model->gfxSkins.find(glnode.skin);
      if (it != _model->gfxSkins.end()) {
        node.skin = it->second;
      } else {
        node.skin =
            _model->gfxSkins
                .emplace(glnode.skin, GFXSkin(_gltf.skins[glnode.skin], _gltf))
                .first->second;
        if (int(node.skin->get().joints.size()) > maxBones)
          throw ActivationError(
              "Too many bones in GLTF model, raise GLTF.MaxBones "
              "property value");
      }
    }

    for (const auto childIndex : glnode.children) {
      const auto &subglnode = _gltf.nodes[childIndex];
      node.children.emplace_back(processNode(context, subglnode));
    }

    return NodeRef(_model->nodes.emplace_back(std::move(node)));
  }

  void readGlobalSettings() {
    std::unique_lock lock(GetGlobals().SettingsMutex);
    auto &vmaxBones = GetGlobals().Settings["GLTF.MaxBones"];
    if (vmaxBones.valueType == CBType::None) {
      vmaxBones = Var(32);
    }
    maxBones = int(Var(vmaxBones));
  }

  int maxBones = 0; // TODO Context
  CBVar loadModel(CBContext *context, const CBVar &input) {
    const auto filename = input.payload.stringValue;
    loadFromFile(filename);

    if (_gltf.defaultScene == -1) {
      throw ActivationError("GLTF model had no default scene.");
    }

    readGlobalSettings();

    if (_model) {
      Model::Var.Release(_model);
      _model = nullptr;
    }
    _model = Model::Var.New();

    const auto &scene = _gltf.scenes[_gltf.defaultScene];
    for (const int gltfNodeIdx : scene.nodes) {
      if (_canceled) // abort if cancellation is flagged
        return Model::Var.Get(_model);

      const auto &glnode = _gltf.nodes[gltfNodeIdx];
      _model->rootNodes.emplace_back(processNode(context, glnode));
    }

    // process animations if needed
    if (_animations.valueType == Seq && _animations.payload.seqValue.len > 0) {
      std::unordered_set<std::string_view> names;
      for (const auto &animation : _animations) {
        names.insert(CBSTRVIEW(animation));
      }
      for (const auto &anim : _gltf.animations) {
        if (_canceled) // abort if cancellation is flagged
          return Model::Var.Get(_model);

        auto it = names.find(anim.name);
        if (it != names.end()) {
          // *it string memory is backed in _animations Var!
          _model->animations.emplace(*it, anim);
        }
      }
    }

    return Model::Var.Get(_model);
  }
};

} // namespace gltf
} // namespace chainblocks