#pragma once

#include "../blocks_macros.hpp"
#include "../chainblocks.hpp"

namespace chainblocks {
struct CoreInfo {
  static inline TypeInfo intType = TypeInfo(CBType::Int);
  static inline TypeInfo floatType = TypeInfo(CBType::Float);
  static inline TypesInfo intInfo = TypesInfo(CBType::Int);
  static inline TypesInfo intVarInfo =
      TypesInfo::FromMany(false, CBType::Int, CBType::ContextVar);
  static inline TypesInfo strInfo = TypesInfo(CBType::String);
  static inline TypesInfo varSeqInfo = TypesInfo(CBType::ContextVar, true);
  static inline TypesInfo anyInfo = TypesInfo(CBType::Any);
  static inline TypesInfo anySeqInfo = TypesInfo(CBType::Seq);
  static inline TypesInfo noneInfo = TypesInfo(CBType::None);
  static inline TypesInfo tableInfo = TypesInfo(CBType::Table);
  static inline TypesInfo floatInfo = TypesInfo(CBType::Float);
  static inline TypesInfo boolInfo = TypesInfo(CBType::Bool);
  static inline TypesInfo blockInfo = TypesInfo(CBType::Block);
  static inline TypeInfo blockType = TypeInfo(CBType::Block);
  static inline TypesInfo blocksInfo = TypesInfo(CBType::Block, true);
  static inline TypeInfo blockSeq = TypeInfo::Sequence(blockType);
  static inline TypesInfo blocksSeqInfo =
      TypesInfo(TypeInfo::Sequence(blockSeq));
  static inline TypesInfo intsInfo = TypesInfo(CBType::Int, true);
  static inline TypesInfo intSeqInfo = TypesInfo(TypeInfo::Sequence(intType));
  static inline TypesInfo floatSeqInfo =
      TypesInfo(TypeInfo::Sequence(floatType));
};

struct Const {
  static inline TypesInfo constTypesInfo = TypesInfo::FromMany(
      true, CBType::Int, CBType::Int2, CBType::Int3, CBType::Int4, CBType::Int8,
      CBType::Int16, CBType::Float, CBType::Float2, CBType::Float3,
      CBType::Float4, CBType::None, CBType::String, CBType::Color,
      CBType::Bool);
  static inline ParamsInfo constParamsInfo = ParamsInfo(
      ParamsInfo::Param("Value", "The constant value to insert in the chain.",
                        CBTypesInfo(constTypesInfo)));

  CBVar _value{};
  TypeInfo _valueInfo{};
  TypeInfo _innerInfo{};

  void destroy() { destroyVar(_value); }

  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::noneInfo); }

  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  static CBParametersInfo parameters() {
    return CBParametersInfo(constParamsInfo);
  }

  void setParam(int index, CBVar value) { cloneVar(_value, value); }

  CBVar getParam(int index) { return _value; }

  CBTypeInfo inferTypes(CBTypeInfo inputType,
                        CBExposedTypesInfo consumableVariables) {
    if (_value.valueType == Seq && _value.payload.seqValue) {
      _innerInfo = TypeInfo(_value.payload.seqValue[0].valueType);
      _valueInfo = TypeInfo::Sequence(_innerInfo);
    } else {
      _valueInfo = TypeInfo(_value.valueType);
    }
    return _valueInfo;
  }

  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    return _value;
  }
};

static ParamsInfo compareParamsInfo = ParamsInfo(
    ParamsInfo::Param("Value", "The value to test against for equality.",
                      CBTypesInfo(CoreInfo::anyInfo)));

struct BaseOpsBin {
  CBVar value{};

  void destroy() { destroyVar(value); }

  CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::boolInfo); }

  CBParametersInfo parameters() { return CBParametersInfo(compareParamsInfo); }

  void setParam(int index, CBVar inValue) {
    switch (index) {
    case 0:
      cloneVar(value, inValue);
      break;
    default:
      break;
    }
  }

  CBVar getParam(int index) {
    auto res = CBVar();
    switch (index) {
    case 0:
      res = value;
      break;
    default:
      break;
    }
    return res;
  }
};

#define LOGIC_OP(NAME, OP)                                                     \
  struct NAME : public BaseOpsBin {                                            \
    ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {     \
      if (input OP value) {                                                    \
        return True;                                                           \
      }                                                                        \
      return False;                                                            \
    }                                                                          \
  };                                                                           \
  RUNTIME_CORE_BLOCK_TYPE(NAME);

LOGIC_OP(Is, ==);
LOGIC_OP(IsNot, !=);
LOGIC_OP(IsMore, >);
LOGIC_OP(IsLess, <);
LOGIC_OP(IsMoreEqual, >=);
LOGIC_OP(IsLessEqual, <=);

#define LOGIC_ANY_SEQ_OP(NAME, OP)                                             \
  struct NAME : public BaseOpsBin {                                            \
    ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {     \
      if (input.valueType == Seq && value.valueType == Seq) {                  \
        auto vlen = stbds_arrlen(value.payload.seqValue);                      \
        auto ilen = stbds_arrlen(input.payload.seqValue);                      \
        if (ilen > vlen)                                                       \
          throw CBException("Failed to compare, input len > value len.");      \
        for (auto i = 0; i < stbds_arrlen(input.payload.seqValue); i++) {      \
          if (input.payload.seqValue[i] OP value.payload.seqValue[i]) {        \
            return True;                                                       \
          }                                                                    \
        }                                                                      \
        return False;                                                          \
      } else if (input.valueType == Seq && value.valueType != Seq) {           \
        for (auto i = 0; i < stbds_arrlen(input.payload.seqValue); i++) {      \
          if (input.payload.seqValue[i] OP value) {                            \
            return True;                                                       \
          }                                                                    \
        }                                                                      \
        return False;                                                          \
      } else if (input.valueType != Seq && value.valueType == Seq) {           \
        for (auto i = 0; i < stbds_arrlen(value.payload.seqValue); i++) {      \
          if (input OP value.payload.seqValue[i]) {                            \
            return True;                                                       \
          }                                                                    \
        }                                                                      \
        return False;                                                          \
      } else if (input.valueType != Seq && value.valueType != Seq) {           \
        if (input OP value) {                                                  \
          return True;                                                         \
        }                                                                      \
        return False;                                                          \
      }                                                                        \
      return False;                                                            \
    }                                                                          \
  };                                                                           \
  RUNTIME_CORE_BLOCK_TYPE(NAME);

#define LOGIC_ALL_SEQ_OP(NAME, OP)                                             \
  struct NAME : public BaseOpsBin {                                            \
    ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {     \
      if (input.valueType == Seq && value.valueType == Seq) {                  \
        auto vlen = stbds_arrlen(value.payload.seqValue);                      \
        auto ilen = stbds_arrlen(input.payload.seqValue);                      \
        if (ilen > vlen)                                                       \
          throw CBException("Failed to compare, input len > value len.");      \
        for (auto i = 0; i < stbds_arrlen(input.payload.seqValue); i++) {      \
          if (!(input.payload.seqValue[i] OP value.payload.seqValue[i])) {     \
            return False;                                                      \
          }                                                                    \
        }                                                                      \
        return True;                                                           \
      } else if (input.valueType == Seq && value.valueType != Seq) {           \
        for (auto i = 0; i < stbds_arrlen(input.payload.seqValue); i++) {      \
          if (!(input.payload.seqValue[i] OP value)) {                         \
            return False;                                                      \
          }                                                                    \
        }                                                                      \
        return True;                                                           \
      } else if (input.valueType != Seq && value.valueType == Seq) {           \
        for (auto i = 0; i < stbds_arrlen(value.payload.seqValue); i++) {      \
          if (!(input OP value.payload.seqValue[i])) {                         \
            return False;                                                      \
          }                                                                    \
        }                                                                      \
        return True;                                                           \
      } else if (input.valueType != Seq && value.valueType != Seq) {           \
        if (!(input OP value)) {                                               \
          return False;                                                        \
        }                                                                      \
        return True;                                                           \
      }                                                                        \
      return False;                                                            \
    }                                                                          \
  };                                                                           \
  RUNTIME_CORE_BLOCK_TYPE(NAME);

LOGIC_ANY_SEQ_OP(Any, ==);
LOGIC_ALL_SEQ_OP(All, ==);
LOGIC_ANY_SEQ_OP(AnyNot, !=);
LOGIC_ALL_SEQ_OP(AllNot, !=);
LOGIC_ANY_SEQ_OP(AnyMore, >);
LOGIC_ALL_SEQ_OP(AllMore, >);
LOGIC_ANY_SEQ_OP(AnyLess, <);
LOGIC_ALL_SEQ_OP(AllLess, <);
LOGIC_ANY_SEQ_OP(AnyMoreEqual, >=);
LOGIC_ALL_SEQ_OP(AllMoreEqual, >=);
LOGIC_ANY_SEQ_OP(AnyLessEqual, <=);
LOGIC_ALL_SEQ_OP(AllLessEqual, <=);

#define LOGIC_OP_DESC(NAME)                                                    \
  RUNTIME_CORE_BLOCK_FACTORY(NAME);                                            \
  RUNTIME_BLOCK_destroy(NAME);                                                 \
  RUNTIME_BLOCK_inputTypes(NAME);                                              \
  RUNTIME_BLOCK_outputTypes(NAME);                                             \
  RUNTIME_BLOCK_parameters(NAME);                                              \
  RUNTIME_BLOCK_setParam(NAME);                                                \
  RUNTIME_BLOCK_getParam(NAME);                                                \
  RUNTIME_BLOCK_activate(NAME);                                                \
  RUNTIME_BLOCK_END(NAME);

struct Sleep {
  static inline ParamsInfo sleepParamsInfo = ParamsInfo(ParamsInfo::Param(
      "Time", "The amount of time in seconds (float) to pause this chain.",
      CBTypesInfo(CoreInfo::floatInfo)));

  double time{};

  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  static CBParametersInfo parameters() {
    return CBParametersInfo(sleepParamsInfo);
  }

  void setParam(int index, CBVar value) { time = value.payload.floatValue; }

  CBVar getParam(int index) { return Var(time); }

  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    cbpause(time);
    return input;
  }
};

struct And {
  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::boolInfo); }

  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::boolInfo); }

  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    if (input.payload.boolValue) {
      // Continue the flow
      return RebaseChain;
    } else {
      // Reason: We are done, input IS FALSE so we FAIL
      return ReturnPrevious;
    }
  }
};

struct Or {
  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::boolInfo); }

  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::boolInfo); }

  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    if (input.payload.boolValue) {
      // Reason: We are done, input IS TRUE so we succeed
      return ReturnPrevious;
    } else {
      // Continue the flow, with the initial input as next input!
      return RebaseChain;
    }
  }
};

struct Stop {
  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }
  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::noneInfo); }
  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    return Var::Stop();
  }
};

struct Restart {
  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }
  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::noneInfo); }
  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    return Var::Restart();
  }
};

struct Return {
  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }
  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::noneInfo); }
  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    return Var::Return();
  }
};

struct VariableBase {
  CBVar *_target = nullptr;
  std::string _name;
  std::string _key;
  ExposedInfo _exposedInfo{};
  bool _global = false; // if we should use a node global
  bool _isTable = false;

  static inline ParamsInfo variableParamsInfo = ParamsInfo(
      ParamsInfo::Param("Name", "The name of the variable.",
                        CBTypesInfo(CoreInfo::strInfo)),
      ParamsInfo::Param("Key",
                        "The key of the value to read/write from/in the table "
                        "(this variable will become a table).",
                        CBTypesInfo(CoreInfo::strInfo)),
      ParamsInfo::Param(
          "Global",
          "If the variable should be shared between chains in the same node.",
          CBTypesInfo(CoreInfo::boolInfo)));

  static CBParametersInfo parameters() {
    return CBParametersInfo(variableParamsInfo);
  }

  void setParam(int index, CBVar value) {
    if (index == 0)
      _name = value.payload.stringValue;
    else if (index == 1) {
      _key = value.payload.stringValue;
      if (_key.size() > 0)
        _isTable = true;
      else
        _isTable = false;
    } else if (index == 2) {
      _global = value.payload.boolValue;
    }
  }

  CBVar getParam(int index) {
    if (index == 0)
      return Var(_name.c_str());
    else if (index == 1)
      return Var(_key.c_str());
    else if (index == 2)
      return Var(_global);
    throw CBException("Param index out of range.");
  }

  bool IsValidVariableName(std::string &variableName) {
    auto idx = variableName.find(
        " "); // we use spaces to concat table variables with keys and such
    if (idx != -1)
      return false;
    return true;
  }
};

struct SetBase : public VariableBase {
  TypeInfo _tableTypeInfo{};
  TypeInfo _tableContentInfo{};

  void cleanup() {
    if (_target) {
      if (_isTable && _target->valueType == Table) {
        // Remove from table
        auto idx = stbds_shgeti(_target->payload.tableValue, _key.c_str());
        if (idx != -1)
          destroyVar(_target->payload.tableValue[idx].value);
        stbds_shdel(_target->payload.tableValue, _key.c_str());
        // Finally free the table if has no values
        if (stbds_shlen(_target->payload.tableValue) == 0) {
          stbds_shfree(_target->payload.tableValue);
          memset(_target, 0x0, sizeof(CBVar));
        }
      } else {
        destroyVar(*_target);
      }
    }
    _target = nullptr;
  }

  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    if (!_target) {
      _target = contextVariable(context, _name.c_str(), _global);
    }
    if (_isTable) {
      if (_target->valueType != Table) {
        // Not initialized yet
        _target->valueType = Table;
        _target->payload.tableValue = nullptr;
        _target->payload.tableLen = -1;
      }

      auto idx = stbds_shgeti(_target->payload.tableValue, _key.c_str());
      if (idx != -1) {
        // Clone on top of it so we recycle memory
        cloneVar(_target->payload.tableValue[idx].value, input);
      } else {
        CBVar tmp{};
        cloneVar(tmp, input);
        stbds_shput(_target->payload.tableValue, _key.c_str(), tmp);
      }
    } else {
      // Clone will try to recyle memory and such
      cloneVar(*_target, input);
    }
    return input;
  }
};

struct Set : public SetBase {
  CBTypeInfo inferTypes(CBTypeInfo inputType,
                        CBExposedTypesInfo consumableVariables) {
    if (!IsValidVariableName(_name)) {
      throw CBException("Set: Invalid variable name!");
    }

    // bake exposed types
    if (_isTable) {
      // we are a table!
      _tableContentInfo = TypeInfo(inputType);
      _tableTypeInfo = TypeInfo::TableRecord(_tableContentInfo, _key.c_str());
      _exposedInfo = ExposedInfo(ExposedInfo::Variable(
          _name.c_str(), "The exposed table.", _tableTypeInfo));
    } else {
      // Set... we warn if the variable is overwritten
      for (auto i = 0; i < stbds_arrlen(consumableVariables); i++) {
        if (consumableVariables[i].name == _name) {
          LOG(INFO) << "Set - Warning: setting an already exposed variable, "
                       "use Update to avoid this warning, variable: "
                    << _name;
        }
      }
      // just a variable!
      _exposedInfo = ExposedInfo(ExposedInfo::Variable(
          _name.c_str(), "The exposed variable.", CBTypeInfo(inputType)));
    }
    return inputType;
  }

  CBExposedTypesInfo exposedVariables() {
    return CBExposedTypesInfo(_exposedInfo);
  }
};

struct Update : public SetBase {
  CBTypeInfo inferTypes(CBTypeInfo inputType,
                        CBExposedTypesInfo consumableVariables) {
    // make sure we update to the same type
    if (_isTable) {
      for (auto i = 0; stbds_arrlen(consumableVariables) > i; i++) {
        auto &name = consumableVariables[i].name;
        if (name == _name &&
            consumableVariables[i].exposedType.basicType == Table &&
            consumableVariables[i].exposedType.tableTypes) {
          auto &tableKeys = consumableVariables[i].exposedType.tableKeys;
          auto &tableTypes = consumableVariables[i].exposedType.tableTypes;
          for (auto y = 0; y < stbds_arrlen(tableKeys); y++) {
            auto &key = tableKeys[y];
            if (_key == key) {
              if (inputType != tableTypes[y]) {
                throw CBException(
                    "Update: error, update is changing the variable type.");
              }
            }
          }
        }
      }
    } else {
      for (auto i = 0; i < stbds_arrlen(consumableVariables); i++) {
        auto &cv = consumableVariables[i];
        if (_name == cv.name) {
          if (inputType != cv.exposedType) {
            throw CBException(
                "Update: error, update is changing the variable type.");
          }
        }
      }
    }

    // bake exposed types
    if (_isTable) {
      // we are a table!
      _tableContentInfo = TypeInfo(inputType);
      _tableTypeInfo = TypeInfo::TableRecord(_tableContentInfo, _key.c_str());
      _exposedInfo = ExposedInfo(ExposedInfo::Variable(
          _name.c_str(), "The exposed table.", _tableTypeInfo));
    } else {
      // just a variable!
      _exposedInfo = ExposedInfo(ExposedInfo::Variable(
          _name.c_str(), "The exposed variable.", CBTypeInfo(inputType)));
    }

    return inputType;
  }

  CBExposedTypesInfo consumedVariables() {
    return CBExposedTypesInfo(_exposedInfo);
  }
};

struct Get : public VariableBase {
  CBVar _defaultValue{};
  CBTypeInfo _defaultType{};

  static inline ParamsInfo getParamsInfo = ParamsInfo(
      variableParamsInfo,
      ParamsInfo::Param(
          "Default",
          "The default value to use to infer types and output if the variable "
          "is not set, key is not there and/or type mismatches.",
          CBTypesInfo(CoreInfo::anyInfo)));

  static CBParametersInfo parameters() {
    return CBParametersInfo(getParamsInfo);
  }

  void setParam(int index, CBVar value) {
    if (index <= 2)
      VariableBase::setParam(index, value);
    else if (index == 3) {
      cloneVar(_defaultValue, value);
    }
  }

  CBVar getParam(int index) {
    if (index <= 2)
      return VariableBase::getParam(index);
    else if (index == 3)
      return _defaultValue;
    throw CBException("Param index out of range.");
  }

  void cleanup() { _target = nullptr; }

  void destroy() { freeDerivedInfo(_defaultType); }

  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::noneInfo); }

  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  CBTypeInfo inferTypes(CBTypeInfo inputType,
                        CBExposedTypesInfo consumableVariables) {
    if (_isTable) {
      for (auto i = 0; stbds_arrlen(consumableVariables) > i; i++) {
        auto &name = consumableVariables[i].name;
        if (name == _name &&
            consumableVariables[i].exposedType.basicType == Table &&
            consumableVariables[i].exposedType.tableTypes) {
          auto &tableKeys = consumableVariables[i].exposedType.tableKeys;
          auto &tableTypes = consumableVariables[i].exposedType.tableTypes;
          for (auto y = 0; y < stbds_arrlen(tableKeys); y++) {
            auto &key = tableKeys[y];
            if (_key == key) {
              return tableTypes[y];
            }
          }
        }
      }
      if (_defaultValue.valueType != None) {
        freeDerivedInfo(_defaultType);
        _defaultType = deriveTypeInfo(_defaultValue);
        return _defaultType;
      } else {
        throw CBException(
            "Get: Could not infer an output type, key not found.");
      }
    } else {
      for (auto i = 0; i < stbds_arrlen(consumableVariables); i++) {
        auto &cv = consumableVariables[i];
        if (_name == cv.name) {
          return cv.exposedType;
        }
      }
    }
    if (_defaultValue.valueType != None) {
      freeDerivedInfo(_defaultType);
      _defaultType = deriveTypeInfo(_defaultValue);
      return _defaultType;
    } else {
      throw CBException("Get: Could not infer an output type.");
    }
  }

  CBExposedTypesInfo consumedVariables() {
    if (_isTable) {
      _exposedInfo = ExposedInfo(
          ExposedInfo::Variable(_name.c_str(), "The consumed table.",
                                CBTypeInfo(CoreInfo::tableInfo)));
    } else {
      _exposedInfo = ExposedInfo(
          ExposedInfo::Variable(_name.c_str(), "The consumed variable.",
                                CBTypeInfo(CoreInfo::anyInfo)));
    }
    return CBExposedTypesInfo(_exposedInfo);
  }

  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    if (!_target) {
      _target = contextVariable(context, _name.c_str(), _global);
    }
    if (_isTable) {
      if (_target->valueType == Table) {
        ptrdiff_t index =
            stbds_shgeti(_target->payload.tableValue, _key.c_str());
        if (index == -1) {
          if (_defaultType.basicType != None) {
            return _defaultValue;
          } else {
            return Var::Restart();
          }
        }
        auto &value = _target->payload.tableValue[index].value;
        if (unlikely(_defaultValue.valueType != None &&
                     value.valueType != _defaultValue.valueType)) {
          return _defaultValue;
        } else {
          return value;
        }
      } else {
        if (_defaultType.basicType != None) {
          return _defaultValue;
        } else {
          return Var::Restart();
        }
      }
    } else {
      auto &value = *_target;
      if (unlikely(_defaultValue.valueType != None &&
                   value.valueType != _defaultValue.valueType)) {
        return _defaultValue;
      } else {
        return value;
      }
    }
  }
};

struct Swap {
  static inline ParamsInfo swapParamsInfo =
      ParamsInfo(ParamsInfo::Param("NameA", "The name of first variable.",
                                   CBTypesInfo(CoreInfo::strInfo)),
                 ParamsInfo::Param("NameB", "The name of second variable.",
                                   CBTypesInfo(CoreInfo::strInfo)));

  std::string _nameA;
  std::string _nameB;
  CBVar *_targetA{};
  CBVar *_targetB{};
  ExposedInfo _exposedInfo;

  void cleanup() {
    _targetA = nullptr;
    _targetB = nullptr;
  }

  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  CBExposedTypesInfo consumedVariables() {
    _exposedInfo = ExposedInfo(
        ExposedInfo::Variable(_nameA.c_str(), "The consumed variable.",
                              CBTypeInfo(CoreInfo::anyInfo)),
        ExposedInfo::Variable(_nameB.c_str(), "The consumed variable.",
                              CBTypeInfo(CoreInfo::anyInfo)));
    return CBExposedTypesInfo(_exposedInfo);
  }

  static CBParametersInfo parameters() {
    return CBParametersInfo(swapParamsInfo);
  }

  void setParam(int index, CBVar value) {
    if (index == 0)
      _nameA = value.payload.stringValue;
    else if (index == 1) {
      _nameB = value.payload.stringValue;
    }
  }

  CBVar getParam(int index) {
    if (index == 0)
      return Var(_nameA.c_str());
    else if (index == 1)
      return Var(_nameB.c_str());
    throw CBException("Param index out of range.");
  }

  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    if (!_targetA) {
      _targetA = contextVariable(context, _nameA.c_str());
      _targetB = contextVariable(context, _nameB.c_str());
    }
    auto tmp = *_targetA;
    *_targetA = *_targetB;
    *_targetB = tmp;
    return input;
  }
};

struct Push : public VariableBase {
  bool _clear = true;
  bool _firstPusher = false; // if we are the initializers!
  bool _tableOwner = false;  // we are the first in the table too!
  CBTypeInfo _seqInfo{};
  CBTypeInfo _seqInnerInfo{};
  CBTypeInfo _tableInfo{};

  static inline ParamsInfo pushParams = ParamsInfo(
      variableParamsInfo,
      ParamsInfo::Param(
          "Clear",
          "If we should clear this sequence at every chain iteration; works "
          "only if this is the first push; default: true.",
          CBTypesInfo(CoreInfo::boolInfo)));

  static CBParametersInfo parameters() { return CBParametersInfo(pushParams); }

  void setParam(int index, CBVar value) {
    if (index <= 2)
      VariableBase::setParam(index, value);
    else if (index == 3) {
      _clear = value.payload.boolValue;
    }
  }

  CBVar getParam(int index) {
    if (index <= 2)
      return VariableBase::getParam(index);
    else if (index == 3)
      return Var(_clear);
    throw CBException("Param index out of range.");
  }

  void destroy() {
    if (_firstPusher) {
      if (_tableInfo.tableKeys)
        stbds_arrfree(_tableInfo.tableKeys);
      if (_tableInfo.tableTypes)
        stbds_arrfree(_tableInfo.tableTypes);
    }
  }

  CBTypeInfo inferTypes(CBTypeInfo inputType,
                        CBExposedTypesInfo consumableVariables) {
    if (!IsValidVariableName(_name)) {
      throw CBException("Set: Invalid variable name!");
    }

    if (_isTable) {
      auto tableFound = false;
      for (auto i = 0; stbds_arrlen(consumableVariables) > i; i++) {
        if (consumableVariables[i].name == _name &&
            consumableVariables[i].exposedType.tableTypes) {
          auto &tableKeys = consumableVariables[i].exposedType.tableKeys;
          auto &tableTypes = consumableVariables[i].exposedType.tableTypes;
          tableFound = true;
          for (auto y = 0; y < stbds_arrlen(tableKeys); y++) {
            if (_key == tableKeys[y] && tableTypes[y].basicType == Seq) {
              return inputType; // found lets escape
            }
          }
        }
      }
      if (!tableFound) {
        // Assume we are the first pushing
        _tableOwner = true;
      }
      _firstPusher = true;
      _tableInfo.basicType = Table;
      if (_tableInfo.tableTypes) {
        stbds_arrfree(_tableInfo.tableTypes);
      }
      if (_tableInfo.tableKeys) {
        stbds_arrfree(_tableInfo.tableKeys);
      }
      _seqInfo.basicType = Seq;
      _seqInnerInfo = inputType;
      _seqInfo.seqType = &_seqInnerInfo;
      stbds_arrpush(_tableInfo.tableTypes, _seqInfo);
      stbds_arrpush(_tableInfo.tableKeys, _key.c_str());
      _exposedInfo = ExposedInfo(ExposedInfo::Variable(
          _name.c_str(), "The exposed table.", CBTypeInfo(_tableInfo)));
    } else {
      for (auto i = 0; i < stbds_arrlen(consumableVariables); i++) {
        auto &cv = consumableVariables[i];
        if (_name == cv.name && cv.exposedType.basicType == Seq) {
          return inputType; // found lets escape
        }
      }
      // Assume we are the first pushing this variable
      _firstPusher = true;
      _seqInfo.basicType = Seq;
      _seqInnerInfo = inputType;
      _seqInfo.seqType = &_seqInnerInfo;
      _exposedInfo = ExposedInfo(ExposedInfo::Variable(
          _name.c_str(), "The exposed sequence.", _seqInfo));
    }
    return inputType;
  }

  CBExposedTypesInfo exposedVariables() {
    if (_firstPusher) {
      return CBExposedTypesInfo(_exposedInfo);
    } else {
      return nullptr;
    }
  }

  void cleanup() {
    if (_firstPusher && _target) {
      if (_isTable && _target->valueType == Table) {
        ptrdiff_t index =
            stbds_shgeti(_target->payload.tableValue, _key.c_str());
        if (index != -1) {
          auto &seq = _target->payload.tableValue[index].value;
          if (seq.valueType == Seq) {
            for (auto i = 0; i < stbds_arrlen(seq.payload.seqValue); i++) {
              destroyVar(seq.payload.seqValue[i]);
            }
          }
          stbds_shdel(_target->payload.tableValue, _key.c_str());
        }
        if (_tableOwner && stbds_shlen(_target->payload.tableValue) == 0) {
          stbds_shfree(_target->payload.tableValue);
          memset(_target, 0x0, sizeof(CBVar));
        }
      } else if (_target->valueType == Seq) {
        for (auto i = 0; i < stbds_arrlen(_target->payload.seqValue); i++) {
          destroyVar(_target->payload.seqValue[i]);
        }
        stbds_arrfree(_target->payload.seqValue);
      }
    }
    _target = nullptr;
  }

  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    if (!_target) {
      _target = contextVariable(context, _name.c_str(), _global);
    }
    if (_isTable) {
      if (_target->valueType != Table) {
        // Not initialized yet
        _target->valueType = Table;
        _target->payload.tableValue = nullptr;
        _target->payload.tableLen = -1;
      }

      ptrdiff_t index = stbds_shgeti(_target->payload.tableValue, _key.c_str());
      if (index == -1) {
        // First empty insertion
        CBVar tmp{};
        stbds_shput(_target->payload.tableValue, _key.c_str(), tmp);
        index = stbds_shgeti(_target->payload.tableValue, _key.c_str());
      }

      auto &seq = _target->payload.tableValue[index].value;

      if (seq.valueType != Seq) {
        seq.valueType = Seq;
        seq.payload.seqValue = nullptr;
        seq.payload.seqLen = -1;
      }

      if (_firstPusher && _clear) {
        auto len = stbds_arrlen(seq.payload.seqValue);
        if (len > 0) {
          if (seq.payload.seqValue[0].valueType >= EndOfBlittableTypes) {
            // Clean allocation garbage in case it's not blittable!
            for (auto i = 0; i < len; i++) {
              destroyVar(seq.payload.seqValue[i]);
            }
          }
          stbds_arrsetlen(seq.payload.seqValue, 0);
        }
      }

      CBVar tmp{};
      cloneVar(tmp, input);
      stbds_arrpush(seq.payload.seqValue, tmp);
    } else {
      if (_target->valueType != Seq) {
        _target->valueType = Seq;
        _target->payload.seqValue = nullptr;
        _target->payload.seqLen = -1;
      }

      if (_firstPusher && _clear) {
        auto len = stbds_arrlen(_target->payload.seqValue);
        if (len > 0) {
          if (_target->payload.seqValue[0].valueType >= EndOfBlittableTypes) {
            // Clean allocation garbage in case it's not blittable!
            for (auto i = 0; i < len; i++) {
              destroyVar(_target->payload.seqValue[i]);
            }
          }
          stbds_arrsetlen(_target->payload.seqValue, 0);
        }
      }

      CBVar tmp{};
      cloneVar(tmp, input);
      stbds_arrpush(_target->payload.seqValue, tmp);
    }
    return input;
  }
};

struct SeqUser : VariableBase {
  bool _blittable = false;

  void cleanup() { _target = nullptr; }

  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  CBExposedTypesInfo consumedVariables() {
    if (_isTable) {
      _exposedInfo = ExposedInfo(
          ExposedInfo::Variable(_name.c_str(), "The consumed table.",
                                CBTypeInfo(CoreInfo::tableInfo)));
    } else {
      _exposedInfo = ExposedInfo(
          ExposedInfo::Variable(_name.c_str(), "The consumed variable.",
                                CBTypeInfo(CoreInfo::anyInfo)));
    }
    return CBExposedTypesInfo(_exposedInfo);
  }
};

struct Count : SeqUser {
  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::noneInfo); }
  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::intInfo); }

  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    if (!_target) {
      _target = contextVariable(context, _name.c_str(), _global);
    }
    if (_isTable) {
      if (_target->valueType != Table) {
        throw CBException("Variable is not a table, failed to Count.");
      }

      ptrdiff_t index = stbds_shgeti(_target->payload.tableValue, _key.c_str());
      if (index == -1) {
        return Var(0);
      }

      auto &seq = _target->payload.tableValue[index].value;
      if (seq.valueType != Seq) {
        return Var(0);
      }

      return Var(int64_t(stbds_arrlen(seq.payload.seqValue)));
    } else {
      if (_target->valueType != Seq) {
        return Var(0);
      }

      return Var(int64_t(stbds_arrlen(_target->payload.seqValue)));
    }
  }
};

struct Clear : SeqUser {
  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    if (!_target) {
      _target = contextVariable(context, _name.c_str(), _global);
    }
    if (_isTable) {
      if (_target->valueType != Table) {
        throw CBException("Variable is not a table, failed to Clear.");
      }

      ptrdiff_t index = stbds_shgeti(_target->payload.tableValue, _key.c_str());
      if (index == -1) {
        return input;
      }

      auto &seq = _target->payload.tableValue[index].value;
      if (seq.valueType != Seq) {
        return input;
      }

      auto len = stbds_arrlen(seq.payload.seqValue);
      if (len > 0) {
        if (seq.payload.seqValue[0].valueType >= EndOfBlittableTypes) {
          // Clean allocation garbage in case it's not blittable!
          for (auto i = 0; i < len; i++) {
            destroyVar(seq.payload.seqValue[i]);
          }
        }
        stbds_arrsetlen(seq.payload.seqValue, 0);
      }
    } else {
      if (_target->valueType != Seq) {
        return input;
      }

      auto len = stbds_arrlen(_target->payload.seqValue);
      if (len > 0) {
        if (_target->payload.seqValue[0].valueType >= EndOfBlittableTypes) {
          // Clean allocation garbage in case it's not blittable!
          for (auto i = 0; i < len; i++) {
            destroyVar(_target->payload.seqValue[i]);
          }
        }
        stbds_arrsetlen(_target->payload.seqValue, 0);
      }
    }
    return input;
  }
};

struct Pop : SeqUser {
  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::noneInfo); }

  CBVar _output{};

  void destroy() { destroyVar(_output); }

  CBTypeInfo inferTypes(CBTypeInfo inputType,
                        CBExposedTypesInfo consumableVariables) {
    if (_isTable) {
      for (auto i = 0; stbds_arrlen(consumableVariables) > i; i++) {
        if (consumableVariables[i].name == _name &&
            consumableVariables[i].exposedType.tableTypes) {
          auto &tableKeys = consumableVariables[i].exposedType.tableKeys;
          auto &tableTypes = consumableVariables[i].exposedType.tableTypes;
          for (auto y = 0; y < stbds_arrlen(tableKeys); y++) {
            if (_key == tableKeys[y] && tableTypes[y].basicType == Seq) {
              if (tableTypes[y].seqType != nullptr &&
                  tableTypes[y].seqType->basicType < EndOfBlittableTypes) {
                _blittable = true;
              } else {
                _blittable = false;
              }
              return *tableTypes[y].seqType; // found lets escape
            }
          }
        }
      }
      throw CBException("Pop: key not found or key value is not a sequence!.");
    } else {
      for (auto i = 0; i < stbds_arrlen(consumableVariables); i++) {
        auto &cv = consumableVariables[i];
        if (_name == cv.name && cv.exposedType.basicType == Seq) {
          if (cv.exposedType.seqType != nullptr &&
              cv.exposedType.seqType->basicType < EndOfBlittableTypes) {
            _blittable = true;
          } else {
            _blittable = false;
          }
          return *cv.exposedType.seqType; // found lets escape
        }
      }
    }
    throw CBException("Variable is not a sequence.");
  }

  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    if (!_target) {
      _target = contextVariable(context, _name.c_str(), _global);
    }
    if (_isTable) {
      if (_target->valueType != Table) {
        throw CBException("Variable (in table) is not a table, failed to Pop.");
      }

      ptrdiff_t index = stbds_shgeti(_target->payload.tableValue, _key.c_str());
      if (index == -1) {
        throw CBException("Record not found in table, failed to Pop.");
      }

      auto &seq = _target->payload.tableValue[index].value;
      if (seq.valueType != Seq) {
        throw CBException(
            "Variable (in table) is not a sequence, failed to Pop.");
      }

      if (stbds_arrlen(seq.payload.seqValue) == 0) {
        throw CBException("Pop: sequence was empty.");
      }

      // Clone and make the var ours, also if not blittable clear actual
      // sequence garbage
      auto pops = stbds_arrpop(seq.payload.seqValue);
      cloneVar(_output, pops);
      if (!_blittable) {
        destroyVar(pops);
      }
      return _output;
    } else {
      if (_target->valueType != Seq) {
        throw CBException("Variable is not a sequence, failed to Pop.");
      }

      if (stbds_arrlen(_target->payload.seqValue) == 0) {
        throw CBException("Pop: sequence was empty.");
      }

      // Clone and make the var ours, also if not blittable clear actual
      // sequence garbage
      auto pops = stbds_arrpop(_target->payload.seqValue);
      cloneVar(_output, pops);
      if (!_blittable) {
        destroyVar(pops);
      }
      return _output;
    }
  }
};

struct Take {
  static inline ParamsInfo indicesParamsInfo = ParamsInfo(ParamsInfo::Param(
      "Indices", "One or multiple indices to filter from a sequence.",
      CBTypesInfo(CoreInfo::intsInfo)));

  CBSeq cachedResult = nullptr;
  CBVar indices{};

  void destroy() {
    if (cachedResult) {
      stbds_arrfree(cachedResult);
    }
    destroyVar(indices);
  }

  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::anySeqInfo); }

  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  static CBParametersInfo parameters() {
    return CBParametersInfo(indicesParamsInfo);
  }

  CBTypeInfo inferTypes(CBTypeInfo inputType,
                        CBExposedTypesInfo consumableVariables) {
    // Figure if we output a sequence or not
    if (indices.valueType == Seq) {
      if (inputType.basicType == Seq) {
        return inputType; // multiple values
      }
    } else {
      if (inputType.basicType == Seq && inputType.seqType) {
        return *inputType.seqType; // single value
      }
    }
    throw CBException("Take expected a sequence as input.");
  }

  void setParam(int index, CBVar value) {
    switch (index) {
    case 0:
      cloneVar(indices, value);
      break;
    default:
      break;
    }
  }

  CBVar getParam(int index) { return indices; }

  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    auto inputLen = stbds_arrlen(input.payload.seqValue);

    if (indices.valueType == Int) {
      auto index = indices.payload.intValue;
      if (index >= inputLen) {
        LOG(ERROR) << "Take out of range! len:" << inputLen
                   << " wanted index: " << index;
        throw CBException("Take out of range!");
      }

      return input.payload.seqValue[index];
    }

    // Else it's a seq
    auto nindices = stbds_arrlen(indices.payload.seqValue);
    stbds_arrsetlen(cachedResult, nindices);
    for (auto i = 0; nindices > i; i++) {
      auto index = indices.payload.seqValue[i].payload.intValue;
      if (index >= inputLen) {
        LOG(ERROR) << "Take out of range! len:" << inputLen
                   << " wanted index: " << index;
        throw CBException("Take out of range!");
      }
      cachedResult[i] = input.payload.seqValue[index];
    }

    CBVar result{};
    result.valueType = Seq;
    result.payload.seqLen = -1;
    result.payload.seqValue = cachedResult;
    return result;
  }
};

struct Limit {
  static inline ParamsInfo indicesParamsInfo = ParamsInfo(ParamsInfo::Param(
      "Max", "How many maximum elements to take from the input sequence.",
      CBTypesInfo(CoreInfo::intInfo)));

  CBSeq _cachedResult = nullptr;
  int64_t _max = 0;

  void destroy() {
    if (_cachedResult) {
      stbds_arrfree(_cachedResult);
    }
  }

  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::anySeqInfo); }

  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  static CBParametersInfo parameters() {
    return CBParametersInfo(indicesParamsInfo);
  }

  CBTypeInfo inferTypes(CBTypeInfo inputType,
                        CBExposedTypesInfo consumableVariables) {
    // Figure if we output a sequence or not
    if (_max > 1) {
      if (inputType.basicType == Seq) {
        return inputType; // multiple values
      }
    } else {
      if (inputType.basicType == Seq && inputType.seqType) {
        return *inputType.seqType; // single value
      }
    }
    throw CBException("Take expected a sequence as input.");
  }

  void setParam(int index, CBVar value) { _max = value.payload.intValue; }

  CBVar getParam(int index) { return Var(_max); }

  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    int64_t inputLen = stbds_arrlen(input.payload.seqValue);

    if (_max == 1) {
      auto index = 0;
      if (index >= inputLen) {
        LOG(ERROR) << "Limit out of range! len:" << inputLen
                   << " wanted index: " << index;
        throw CBException("Limit out of range!");
      }

      return input.payload.seqValue[index];
    }

    // Else it's a seq
    auto nindices = std::min(inputLen, _max);
    stbds_arrsetlen(_cachedResult, nindices);
    for (auto i = 0; i < nindices; i++) {
      _cachedResult[i] = input.payload.seqValue[i];
    }

    CBVar result{};
    result.valueType = Seq;
    result.payload.seqLen = -1;
    result.payload.seqValue = _cachedResult;
    return result;
  }
};

struct BlocksUser {
  CBVar _blocks{};
  CBValidationResult _chainValidation{};

  void destroy() {
    if (_blocks.valueType == Seq) {
      for (auto i = 0; i < stbds_arrlen(_blocks.payload.seqValue); i++) {
        auto &blk = _blocks.payload.seqValue[i].payload.blockValue;
        blk->cleanup(blk);
        blk->destroy(blk);
      }
    } else if (_blocks.valueType == Block) {
      _blocks.payload.blockValue->cleanup(_blocks.payload.blockValue);
      _blocks.payload.blockValue->destroy(_blocks.payload.blockValue);
    }
    destroyVar(_blocks);
    stbds_arrfree(_chainValidation.exposedInfo);
  }

  void cleanup() {
    if (_blocks.valueType == Seq) {
      for (auto i = 0; i < stbds_arrlen(_blocks.payload.seqValue); i++) {
        auto &blk = _blocks.payload.seqValue[i].payload.blockValue;
        blk->cleanup(blk);
      }
    } else if (_blocks.valueType == Block) {
      _blocks.payload.blockValue->cleanup(_blocks.payload.blockValue);
    }
  }

  CBTypeInfo inferTypes(CBTypeInfo inputType, CBExposedTypesInfo consumables) {
    // Free any previous result!
    stbds_arrfree(_chainValidation.exposedInfo);
    _chainValidation.exposedInfo = nullptr;

    std::vector<CBlock *> blocks;
    if (_blocks.valueType == Block) {
      blocks.push_back(_blocks.payload.blockValue);
    } else {
      for (auto i = 0; i < stbds_arrlen(_blocks.payload.seqValue); i++) {
        blocks.push_back(_blocks.payload.seqValue[i].payload.blockValue);
      }
    }
    _chainValidation = validateConnections(
        blocks,
        [](const CBlock *errorBlock, const char *errorTxt, bool nonfatalWarning,
           void *userData) {
          if (!nonfatalWarning) {
            LOG(ERROR) << "Failed inner chain validation, error: " << errorTxt;
            throw CBException("Failed inner chain validation.");
          } else {
            LOG(INFO) << "Warning during inner chain validation: " << errorTxt;
          }
        },
        this, inputType, consumables);

    return inputType;
  }

  CBExposedTypesInfo exposedVariables() { return _chainValidation.exposedInfo; }
};

struct Repeat : public BlocksUser {
  std::string _ctxVar;
  CBVar *_ctxTimes = nullptr;
  int64_t _times = 0;
  bool _forever = false;
  ExposedInfo _consumedInfo{};

  void cleanup() {
    BlocksUser::cleanup();
    _ctxTimes = nullptr;
  }

  static inline ParamsInfo repeatParamsInfo = ParamsInfo(
      ParamsInfo::Param("Action", "The blocks to repeat.",
                        CBTypesInfo(CoreInfo::blocksInfo)),
      ParamsInfo::Param("Times", "How many times we should repeat the action.",
                        CBTypesInfo(CoreInfo::intVarInfo)),
      ParamsInfo::Param("Forever", "If we should repeat the action forever.",
                        CBTypesInfo(CoreInfo::boolInfo)));

  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::anyInfo); }

  static CBParametersInfo parameters() {
    return CBParametersInfo(repeatParamsInfo);
  }

  void setParam(int index, CBVar value) {
    switch (index) {
    case 0:
      cloneVar(_blocks, value);
      break;
    case 1:
      if (value.valueType == Int) {
        _ctxVar.clear();
        _times = value.payload.intValue;
      } else {
        _ctxVar.assign(value.payload.stringValue);
        _ctxTimes = nullptr;
      }
      break;
    case 2:
      _forever = value.payload.boolValue;
      break;
    default:
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
      return _blocks;
    case 1:
      if (_ctxVar.size() == 0) {
        return Var(_times);
      } else {
        auto ctxTimes = Var(_ctxVar);
        ctxTimes.valueType = ContextVar;
        return ctxTimes;
      }
    case 2:
      return Var(_forever);
    default:
      break;
    }
    throw CBException("Parameter out of range.");
  }

  CBExposedTypesInfo consumedVariables() {
    if (_ctxVar.size() == 0) {
      return nullptr;
    } else {
      _consumedInfo = ExposedInfo(ExposedInfo::Variable(
          _ctxVar.c_str(), "The Int number of repeats variable.",
          CBTypeInfo(CoreInfo::intInfo)));
      return CBExposedTypesInfo(_consumedInfo);
    }
  }

  ALWAYS_INLINE CBVar activate(CBContext *context, const CBVar &input) {
    auto repeats = _forever ? 1 : _times;

    if (_ctxVar.size()) {
      if (!_ctxTimes)
        _ctxTimes = contextVariable(context, _ctxVar.c_str());
      repeats = _ctxTimes->payload.intValue;
    }

    while (repeats) {
      CBVar repeatOutput{};
      repeatOutput.valueType = None;
      repeatOutput.payload.chainState = CBChainState::Continue;
      auto state = activateBlocks(_blocks.payload.seqValue, context, input,
                                  repeatOutput);
      if (unlikely(state == FlowState::Stopping)) {
        return StopChain;
      } else if (unlikely(state == FlowState::Returning)) {
        break;
      }

      if (!_forever)
        repeats--;
    }
    return input;
  }
};

struct JointOp {
  std::vector<CBSeq> _multiSortColumns;
  CBVar _columns{};

  static CBTypesInfo inputTypes() { return CBTypesInfo(CoreInfo::anySeqInfo); }
  static CBTypesInfo outputTypes() { return CBTypesInfo(CoreInfo::anySeqInfo); }

  static inline ParamsInfo joinOpParams = ParamsInfo(ParamsInfo::Param(
      "Join",
      "Other columns to join sort/filter using the input (they must be "
      "of the same length).",
      CBTypesInfo(CoreInfo::varSeqInfo)));

  void setParam(int index, CBVar value) {
    switch (index) {
    case 0:
      cloneVar(_columns, value);
      // resets vars fetch in activate
      _multiSortColumns.clear();
      break;
    default:
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
      return _columns;
    default:
      break;
    }
    throw CBException("Parameter out of range.");
  }

  void cleanup() { _multiSortColumns.clear(); }

  void ensureJoinSetup(CBContext *context, const CBVar &input) {
    if (_columns.valueType != None && _multiSortColumns.size() == 0) {
      auto len = stbds_arrlen(input.payload.seqValue);
      if (_columns.valueType == Seq) {
        auto seq = IterableSeq(_columns.payload.seqValue);
        for (auto &col : seq) {
          auto target = contextVariable(context, col.payload.stringValue);
          if (target && target->valueType == Seq) {
            auto mseqLen = stbds_arrlen(target->payload.seqValue);
            if (len != mseqLen) {
              throw CBException(
                  "Sort: All the sequences to be sorted must have "
                  "the same length as the input sequence.");
            }
            _multiSortColumns.push_back(target->payload.seqValue);
          }
        }
      } else if (_columns.valueType ==
                 ContextVar) { // normal single context var
        auto target = contextVariable(context, _columns.payload.stringValue);
        if (target && target->valueType == Seq) {
          auto mseqLen = stbds_arrlen(target->payload.seqValue);
          if (len != mseqLen) {
            throw CBException("Sort: All the sequences to be sorted must have "
                              "the same length as the input sequence.");
          }
          _multiSortColumns.push_back(target->payload.seqValue);
        }
      }
    }
  }
};

RUNTIME_CORE_BLOCK_TYPE(Const);
RUNTIME_CORE_BLOCK_TYPE(Sleep);
RUNTIME_CORE_BLOCK_TYPE(And);
RUNTIME_CORE_BLOCK_TYPE(Or);
RUNTIME_CORE_BLOCK_TYPE(Stop);
RUNTIME_CORE_BLOCK_TYPE(Restart);
RUNTIME_CORE_BLOCK_TYPE(Return);
RUNTIME_CORE_BLOCK_TYPE(Set);
RUNTIME_CORE_BLOCK_TYPE(Update);
RUNTIME_CORE_BLOCK_TYPE(Get);
RUNTIME_CORE_BLOCK_TYPE(Swap);
RUNTIME_CORE_BLOCK_TYPE(Take);
RUNTIME_CORE_BLOCK_TYPE(Limit);
RUNTIME_CORE_BLOCK_TYPE(Push);
RUNTIME_CORE_BLOCK_TYPE(Pop);
RUNTIME_CORE_BLOCK_TYPE(Clear);
RUNTIME_CORE_BLOCK_TYPE(Count);
RUNTIME_CORE_BLOCK_TYPE(Repeat);
}; // namespace chainblocks