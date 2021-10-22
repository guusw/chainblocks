/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2021 Fragcolor Pte. Ltd. */

#include "../bgfx.hpp"
#include "blocks/shared.hpp"
#include "linalg_shim.hpp"
#include "runtime.hpp"

#include <deque>
#include <filesystem>
#include <optional>
namespace fs = std::filesystem;
using LastWriteTime = decltype(fs::last_write_time(fs::path()));

#include <boost/algorithm/string.hpp>

#include <nlohmann/json.hpp>
#include <stb_image.h>
#include <stb_image_write.h>

#include "gltf.hpp"

namespace chainblocks {
namespace gltf {

struct Draw : public BGFX::BaseConsumer {
  ParamVar _model{};
  ParamVar _materials{};
  OwnedVar _rootChain{};
  std::array<CBExposedTypeInfo, 5> _required;
  std::unordered_map<size_t,
                     std::optional<std::pair<const CBVar *, const CBVar *>>>
      _matsCache;

  struct AnimUniform {
    bgfx::UniformHandle handle;

    AnimUniform() {
      // retreive the maxBones setting here
      int maxBones = 0;
      {
        std::unique_lock lock(GetGlobals().SettingsMutex);
        auto &vmaxBones = GetGlobals().Settings["GLTF.MaxBones"];
        if (vmaxBones.valueType == CBType::None) {
          vmaxBones = Var(32);
        }
        maxBones = int(Var(vmaxBones));
      }

      handle = bgfx::createUniform("u_anim", bgfx::UniformType::Mat4, maxBones);
    }

    ~AnimUniform() {
      if (handle.idx != bgfx::kInvalidHandle) {
        bgfx::destroy(handle);
      }
    }
  };
  Shared<AnimUniform> _animUniform{};

  static inline Types MaterialTableValues3{{BGFX::Texture::SeqType}};
  static inline std::array<CBString, 1> MaterialTableKeys3{"Textures"};
  static inline Type MaterialTableType3 =
      Type::TableOf(MaterialTableValues3, MaterialTableKeys3);
  static inline Type MaterialsTableType3 = Type::TableOf(MaterialTableType3);
  static inline Type MaterialsTableVarType3 =
      Type::VariableOf(MaterialsTableType3);

  static inline Types MaterialTableValues2{
      {BGFX::ShaderHandle::ObjType, BGFX::Texture::SeqType}};
  static inline std::array<CBString, 2> MaterialTableKeys2{"Shader",
                                                           "Textures"};
  static inline Type MaterialTableType2 =
      Type::TableOf(MaterialTableValues2, MaterialTableKeys2);
  static inline Type MaterialsTableType2 = Type::TableOf(MaterialTableType2);
  static inline Type MaterialsTableVarType2 =
      Type::VariableOf(MaterialsTableType2);

  static inline Types MaterialTableValues{{BGFX::ShaderHandle::ObjType}};
  static inline std::array<CBString, 1> MaterialTableKeys{"Shader"};
  static inline Type MaterialTableType =
      Type::TableOf(MaterialTableValues, MaterialTableKeys);
  static inline Type MaterialsTableType = Type::TableOf(MaterialTableType);
  static inline Type MaterialsTableVarType =
      Type::VariableOf(MaterialsTableType);

  static CBTypesInfo inputTypes() { return CoreInfo::Float4x4Types; }
  static CBTypesInfo outputTypes() { return CoreInfo::Float4x4Types; }

  static inline Parameters Params{
      {"Model",
       CBCCSTR("The GLTF model to render."),
       {Model::Type, Model::VarType}},
      {"Materials",
       CBCCSTR("The materials override table, to override the default PBR "
               "metallic-roughness by primitive material name. The table must "
               "be like {Material-Name <name> {Shader <shader> Textures "
               "[<texture>]}} - Textures can be omitted."),
       {CoreInfo::NoneType, MaterialsTableType, MaterialsTableVarType,
        MaterialsTableType2, MaterialsTableVarType2, MaterialsTableType3,
        MaterialsTableVarType3}},
      {"Controller",
       CBCCSTR("The animation controller chain to use. Requires a skinned "
               "model. Will clone and run a copy of the chain."),
       {CoreInfo::NoneType, CoreInfo::ChainType}}};
  static CBParametersInfo parameters() { return Params; }

  void setParam(int index, const CBVar &value) {
    switch (index) {
    case 0:
      _model = value;
      break;
    case 1:
      _materials = value;
      break;
    case 2:
      _rootChain = value;
      break;
    default:
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
      return _model;
    case 1:
      return _materials;
    case 2:
      return _rootChain;
    default:
      throw InvalidParameterIndex();
    }
  }

  CBExposedTypesInfo requiredVariables() {
    int idx = 0;
    _required[idx] = BGFX::BaseConsumer::ContextInfo;
    idx++;

    if (_model.isVariable()) {
      _required[idx].name = _model.variableName();
      _required[idx].help = CBCCSTR("The required model.");
      _required[idx].exposedType = Model::Type;
      idx++;
    }
    if (_materials.isVariable()) {
      _required[idx].name = _materials.variableName();
      _required[idx].help = CBCCSTR("The required materials table.");
      _required[idx].exposedType = MaterialsTableType;
      idx++;
      // OR
      _required[idx].name = _materials.variableName();
      _required[idx].help = CBCCSTR("The required materials table.");
      _required[idx].exposedType = MaterialsTableType2;
      idx++;
      // OR
      _required[idx].name = _materials.variableName();
      _required[idx].help = CBCCSTR("The required materials table.");
      _required[idx].exposedType = MaterialsTableType3;
      idx++;
    }
    return {_required.data(), uint32_t(idx), 0};
  }

  CBTypeInfo compose(const CBInstanceData &data) {
    BGFX::BaseConsumer::compose(data);

    if (data.inputType.seqTypes.elements[0].basicType == CBType::Seq) {
      OVERRIDE_ACTIVATE(data, activate);
    } else {
      OVERRIDE_ACTIVATE(data, activateSingle);
    }

    return data.inputType;
  }

  void warmup(CBContext *context) {
    BGFX::BaseConsumer::_warmup(context);

    _model.warmup(context);
    _materials.warmup(context);
  }

  void cleanup() {
    _matsCache.clear();
    _model.cleanup();
    _materials.cleanup();

    BGFX::BaseConsumer::_cleanup();
  }

  void renderNodeSubmit(BGFX::Context *ctx, const Node &node,
                        const CBTable *mats, bool instanced) {
    if (node.mesh) {
      for (const auto &primsRef : node.mesh->get().primitives) {
        const auto &prims = primsRef.get();
        if (prims.vb.idx != bgfx::kInvalidHandle) {
          const auto currentView = ctx->currentView();

          uint64_t state = prims.stateFlags | BGFX_STATE_WRITE_RGB |
                           BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
                           BGFX_STATE_DEPTH_TEST_LESS;

          if (!prims.material || !(*prims.material).get().doubleSided) {
            if constexpr (BGFX::CurrentRenderer == BGFX::Renderer::OpenGL) {
              // workaround for flipped Y render to textures
              if (currentView.id > 0) {
                state |= BGFX_STATE_CULL_CCW;
              } else {
                state |= BGFX_STATE_CULL_CW;
              }
            } else {
              state |= BGFX_STATE_CULL_CW;
            }
          }

          bgfx::setState(state);

          bgfx::setVertexBuffer(0, prims.vb);
          if (prims.ib.idx != bgfx::kInvalidHandle) {
            bgfx::setIndexBuffer(prims.ib);
          }

          bgfx::ProgramHandle handle = BGFX_INVALID_HANDLE;

          if (prims.material) {
            const auto &material = (*prims.material).get();
            const CBVar *pshader = nullptr;
            const CBVar *ptextures = nullptr;

            if (mats) {
              const auto it = _matsCache.find(material.hash);
              if (it != _matsCache.end()) {
                if (it->second) {
                  pshader = it->second->first;
                  ptextures = it->second->second;
                }
              } else {
                // not found, let's cache it as well
                const auto override =
                    mats->api->tableAt(*mats, material.name.c_str());
                if (override->valueType == CBType::Table) {
                  const auto &records = override->payload.tableValue;
                  pshader = records.api->tableAt(records, "Shader");
                  ptextures = records.api->tableAt(records, "Textures");
                  // add to quick cache map by integer
                  _matsCache[material.hash] =
                      std::make_pair(pshader, ptextures);
                } else {
                  _matsCache[material.hash].reset();
                }
              }
            }

            // try override shader, if not use the default PBR shader
            if (pshader && pshader->valueType == CBType::Object) {
              // we got the shader
              const auto shader = reinterpret_cast<BGFX::ShaderHandle *>(
                  pshader->payload.objectValue);
              handle = shader->handle;
            } else if (material.shader) {
              handle = instanced ? material.shader->handleInstanced
                                 : material.shader->handle;
            }

            // if textures are empty, use the ones we loaded during Load
            if (ptextures && ptextures->valueType == CBType::Seq &&
                ptextures->payload.seqValue.len > 0) {
              const auto textures = ptextures->payload.seqValue;
              for (uint32_t i = 0; i < textures.len; i++) {
                const auto texture = reinterpret_cast<BGFX::Texture *>(
                    textures.elements[i].payload.objectValue);
                bgfx::setTexture(uint8_t(i), ctx->getSampler(i),
                                 texture->handle);
              }
            } else {
              uint8_t samplerSlot = 0;
              if (material.baseColorTexture) {
                bgfx::setTexture(samplerSlot, ctx->getSampler(samplerSlot),
                                 material.baseColorTexture->handle);
                samplerSlot++;
              }
              if (material.normalTexture) {
                bgfx::setTexture(samplerSlot, ctx->getSampler(samplerSlot),
                                 material.normalTexture->handle);
                samplerSlot++;
              }
            }
          }

          if (handle.idx == bgfx::kInvalidHandle) {
            const std::string *matName;
            if (prims.material) {
              const auto &material = (*prims.material).get();
              matName = &material.name;
            } else {
              matName = nullptr;
            }
            CBLOG_ERROR("Rendering a primitive with invalid shader handle, "
                        "mesh: {} material: {} mats table: {}",
                        node.mesh->get().name,
                        (matName ? matName->c_str() : "<no material>"),
                        _materials.get());
            throw ActivationError(
                "Rendering a primitive with invalid shader handle");
          }

          bgfx::submit(currentView.id, handle);
        }
      }
    }
  }

  void renderNode(BGFX::Context *ctx, const Node &node,
                  const linalg::aliases::float4x4 &parentTransform,
                  const CBTable *mats, bool instanced) {
    const auto transform = linalg::mul(parentTransform, node.transform);

    bgfx::Transform t;
    // using allocTransform to avoid an extra copy
    auto idx = bgfx::allocTransform(&t, 1);
    memcpy(&t.data[0], &transform.x, sizeof(float) * 4);
    memcpy(&t.data[4], &transform.y, sizeof(float) * 4);
    memcpy(&t.data[8], &transform.z, sizeof(float) * 4);
    memcpy(&t.data[12], &transform.w, sizeof(float) * 4);
    bgfx::setTransform(idx, 1);

    renderNodeSubmit(ctx, node, mats, instanced);

    for (const auto &snode : node.children) {
      renderNode(ctx, snode, transform, mats, instanced);
    }
  }

  CBVar activate(CBContext *context, const CBVar &input) {
    const auto instances = input.payload.seqValue.len;
    constexpr uint16_t stride = 64; // matrix 4x4
    if (bgfx::getAvailInstanceDataBuffer(instances, stride) != instances) {
      throw ActivationError("Instance buffer overflow");
    }

    bgfx::InstanceDataBuffer idb;
    bgfx::allocInstanceDataBuffer(&idb, instances, stride);
    uint8_t *data = idb.data;

    for (uint32_t i = 0; i < instances; i++) {
      float *mat = reinterpret_cast<float *>(data);
      CBVar &vmat = input.payload.seqValue.elements[i];

      if (vmat.payload.seqValue.len != 4) {
        throw ActivationError("Invalid Matrix4x4 input, should Float4 x 4.");
      }

      memcpy(&mat[0], &vmat.payload.seqValue.elements[0].payload.float4Value,
             sizeof(float) * 4);
      memcpy(&mat[4], &vmat.payload.seqValue.elements[1].payload.float4Value,
             sizeof(float) * 4);
      memcpy(&mat[8], &vmat.payload.seqValue.elements[2].payload.float4Value,
             sizeof(float) * 4);
      memcpy(&mat[12], &vmat.payload.seqValue.elements[3].payload.float4Value,
             sizeof(float) * 4);

      data += stride;
    }

    bgfx::setInstanceDataBuffer(&idb);

    auto *ctx =
        reinterpret_cast<BGFX::Context *>(_bgfxCtx->payload.objectValue);

    const auto &modelVar = _model.get();
    const auto model = reinterpret_cast<Model *>(modelVar.payload.objectValue);
    if (!model)
      return input;

    const auto &matsVar = _materials.get();
    const auto mats = matsVar.valueType != CBType::None
                          ? &matsVar.payload.tableValue
                          : nullptr;

    auto rootTransform = Mat4::Identity();
    for (const auto &nodeRef : model->rootNodes) {
      renderNode(ctx, nodeRef.get(), rootTransform, mats, true);
    }

    return input;
  }

  CBVar activateSingle(CBContext *context, const CBVar &input) {
    auto *ctx =
        reinterpret_cast<BGFX::Context *>(_bgfxCtx->payload.objectValue);

    const auto &modelVar = _model.get();
    const auto model = reinterpret_cast<Model *>(modelVar.payload.objectValue);
    if (!model)
      return input;

    const auto &matsVar = _materials.get();
    const auto mats = matsVar.valueType != CBType::None
                          ? &matsVar.payload.tableValue
                          : nullptr;

    auto rootTransform =
        reinterpret_cast<Mat4 *>(&input.payload.seqValue.elements[0]);
    for (const auto &nodeRef : model->rootNodes) {
      renderNode(ctx, nodeRef.get(), *rootTransform, mats, false);
    }

    return input;
  }
};

} // namespace gltf
} // namespace chainblocks
