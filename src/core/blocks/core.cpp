/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2019 Fragcolor Pte. Ltd. */

#include "../runtime.hpp"
#include "pdqsort.h"
#include "utility.hpp"
#include <boost/algorithm/string.hpp>
#include <chrono>

namespace chainblocks {
struct JointOp {
  std::vector<CBVar *> _multiSortColumns;

  CBVar _inputVar{};
  CBVar *_input = nullptr;
  CBVar _columns{};

  static CBTypesInfo inputTypes() { return CoreInfo::NoneType; }
  static CBTypesInfo outputTypes() { return CoreInfo::AnySeqType; }

  static inline Type anyVarSeq = Type::VariableOf(CoreInfo::AnySeqType);
  static inline Type anyVarSeqSeq = Type::SeqOf(anyVarSeq);

  static inline Parameters joinOpParams{
      {"From",
       CBCCSTR("The name of the sequence variable to edit in place."),
       {anyVarSeq}},
      {"Join",
       CBCCSTR("Other columns to join sort/filter using the input (they "
               "must be of the same length)."),
       {anyVarSeq, anyVarSeqSeq}}};

  void setParam(int index, const CBVar &value) {
    switch (index) {
    case 0:
      cloneVar(_inputVar, value);
      break;
    case 1:
      cloneVar(_columns, value);
      cleanup();
      break;
    default:
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
      return _inputVar;
    case 1:
      return _columns;
    default:
      break;
    }
    throw CBException("Parameter out of range.");
  }

  void cleanup() {
    for (auto ref : _multiSortColumns) {
      releaseVariable(ref);
    }
    _multiSortColumns.clear();

    if (_input) {
      releaseVariable(_input);
      _input = nullptr;
    }
  }

  void warmup(CBContext *context) {
    if (!_input) {
      _input = referenceVariable(context, _inputVar.payload.stringValue);
    }
  }

  void ensureJoinSetup(CBContext *context) {
    if (_columns.valueType != None) {
      auto len = _input->payload.seqValue.len;
      if (_multiSortColumns.size() == 0) {
        if (_columns.valueType == Seq) {
          for (const auto &col : _columns) {
            auto target = referenceVariable(context, col.payload.stringValue);
            if (target && target->valueType == Seq) {
              auto mseqLen = target->payload.seqValue.len;
              if (len != mseqLen) {
                throw ActivationError(
                    "JointOp: All the sequences to be processed must have "
                    "the same length as the input sequence.");
              }
              _multiSortColumns.push_back(target);
            }
          }
        } else if (_columns.valueType ==
                   ContextVar) { // normal single context var
          auto target =
              referenceVariable(context, _columns.payload.stringValue);
          if (target && target->valueType == Seq) {
            auto mseqLen = target->payload.seqValue.len;
            if (len != mseqLen) {
              throw ActivationError(
                  "JointOp: All the sequences to be processed must have "
                  "the same length as the input sequence.");
            }
            _multiSortColumns.push_back(target);
          }
        }
      } else {
        for (const auto &seqVar : _multiSortColumns) {
          const auto &seq = seqVar->payload.seqValue;
          auto mseqLen = seq.len;
          if (len != mseqLen) {
            throw ActivationError(
                "JointOp: All the sequences to be processed must have "
                "the same length as the input sequence.");
          }
        }
      }
    }
  }
};

struct ActionJointOp : public JointOp {
  BlocksVar _blks{};
  void cleanup() {
    _blks.cleanup();
    JointOp::cleanup();
  }

  void warmup(CBContext *ctx) {
    JointOp::warmup(ctx);
    _blks.warmup(ctx);
  }
};

struct Sort : public ActionJointOp {
  std::vector<CBVar> _multiSortKeys;
  bool _desc = false;

  void setup() { blocksKeyFn._bu = this; }

  static inline Parameters paramsInfo{
      joinOpParams,
      {{"Desc",
        CBCCSTR(
            "If sorting should be in descending order, defaults ascending."),
        {CoreInfo::BoolType}},
       {"Key",
        CBCCSTR("The blocks to use to transform the collection's items "
                "before they are compared. Can be None."),
        {CoreInfo::BlocksOrNone}}}};

  static CBParametersInfo parameters() { return paramsInfo; }

  void setParam(int index, const CBVar &value) {
    switch (index) {
    case 0:
    case 1:
      return JointOp::setParam(index, value);
    case 2:
      _desc = value.payload.boolValue;
      break;
    case 3:
      _blks = value;
      break;
    default:
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
    case 1:
      return JointOp::getParam(index);
    case 2:
      return Var(_desc);
    case 3:
      return _blks;
    default:
      break;
    }
    throw CBException("Parameter out of range.");
  }

  CBTypeInfo compose(CBInstanceData &data) {
    if (_inputVar.valueType != ContextVar)
      throw CBException("From variable was empty!");

    CBExposedTypeInfo info{};
    for (auto &reference : data.shared) {
      if (strcmp(reference.name, _inputVar.payload.stringValue) == 0) {
        info = reference;
        goto found;
      }
    }

    throw CBException("From variable not found!");

  found:
    // need to replace input type of inner chain with inner of seq
    if (info.exposedType.seqTypes.len != 1)
      throw CBException("From variable is not a single type Seq.");

    auto inputType = info.exposedType;
    data.inputType = info.exposedType.seqTypes.elements[0];
    _blks.compose(data);
    return inputType;
  }

  struct {
    bool operator()(const CBVar &a, const CBVar &b) const { return a > b; }
  } sortAsc;

  struct {
    bool operator()(const CBVar &a, const CBVar &b) const { return a < b; }
  } sortDesc;

  struct {
    const CBVar &operator()(const CBVar &a) { return a; }
  } noopKeyFn;

  struct {
    Sort *_bu;
    CBContext *_ctx;
    CBVar _o;

    const CBVar &operator()(const CBVar &a) {
      _bu->_blks.activate(_ctx, a, _o);
      return _o;
    }
  } blocksKeyFn;

  template <class Compare, class KeyFn>
  void insertSort(CBVar seq[], int64_t n, Compare comp, KeyFn keyfn) {
    int64_t i, j;
    CBVar key{};
    for (i = 1; i < n; i++) {
      // main
      key = seq[i];
      // joined seqs
      _multiSortKeys.clear();
      for (const auto &seqVar : _multiSortColumns) {
        const auto &col = seqVar->payload.seqValue;
        if (col.len != n) {
          throw ActivationError(
              "Sort: All the sequences to be processed must have "
              "the same length as the input sequence.");
        }
        _multiSortKeys.push_back(col.elements[i]);
      }
      j = i - 1;
      // notice no &, we WANT to copy
      auto b = keyfn(key);
      while (j >= 0 && comp(keyfn(seq[j]), b)) {
        // main
        seq[j + 1] = seq[j];
        // joined seqs
        for (const auto &seqVar : _multiSortColumns) {
          const auto &col = seqVar->payload.seqValue;
          col.elements[j + 1] = col.elements[j];
        }
        // main + join
        j = j - 1;
      }
      // main
      seq[j + 1] = key;
      // joined seq
      auto z = 0;
      for (const auto &seqVar : _multiSortColumns) {
        const auto &col = seqVar->payload.seqValue;
        col.elements[j + 1] = _multiSortKeys[z++];
      }
    }
  }

  CBVar activate(CBContext *context, const CBVar &input) {
    JointOp::ensureJoinSetup(context);
    // Sort in plac
    int64_t len = int64_t(_input->payload.seqValue.len);
    if (_blks) {
      blocksKeyFn._ctx = context;
      if (!_desc) {
        insertSort(_input->payload.seqValue.elements, len, sortAsc,
                   blocksKeyFn);
      } else {
        insertSort(_input->payload.seqValue.elements, len, sortDesc,
                   blocksKeyFn);
      }
    } else {
      if (!_desc) {
        insertSort(_input->payload.seqValue.elements, len, sortAsc, noopKeyFn);
      } else {
        insertSort(_input->payload.seqValue.elements, len, sortDesc, noopKeyFn);
      }
    }
    return *_input;
  }
};

struct Remove : public ActionJointOp {
  bool _fast = false;

  static inline Parameters paramsInfo{
      joinOpParams,
      {{"Predicate",
        CBCCSTR("The blocks to use as predicate, if true the "
                "item will be popped from the sequence."),
        {CoreInfo::Blocks}},
       {"Unordered",
        CBCCSTR("Turn on to remove items very quickly but will "
                "not preserve the sequence items order."),
        {CoreInfo::BoolType}}}};

  static CBParametersInfo parameters() { return paramsInfo; }

  void setParam(int index, const CBVar &value) {
    switch (index) {
    case 0:
    case 1:
      return JointOp::setParam(index, value);
    case 2:
      _blks = value;
      break;
    case 3:
      _fast = value.payload.boolValue;
      break;
    default:
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
    case 1:
      return JointOp::getParam(index);
    case 2:
      return _blks;
    case 3:
      return Var(_fast);
    default:
      break;
    }
    throw CBException("Parameter out of range.");
  }

  CBTypeInfo compose(CBInstanceData &data) {
    if (_inputVar.valueType != ContextVar)
      throw CBException("From variable was empty!");

    CBExposedTypeInfo info{};
    for (auto &reference : data.shared) {
      if (strcmp(reference.name, _inputVar.payload.stringValue) == 0) {
        info = reference;
        goto found;
      }
    }

    throw CBException("From variable not found!");

  found:
    // need to replace input type of inner chain with inner of seq
    if (info.exposedType.seqTypes.len != 1)
      throw CBException("From variable is not a single type Seq.");

    auto inputType = info.exposedType;
    data.inputType = info.exposedType.seqTypes.elements[0];
    const auto pres = _blks.compose(data);
    if (pres.outputType.basicType != CBType::Bool) {
      throw ComposeError("Remove Predicate should output a boolean value.");
    }
    return inputType;
  }

  CBVar activate(CBContext *context, const CBVar &input) {
    JointOp::ensureJoinSetup(context);
    // Remove in place, will possibly remove any sorting!
    CBVar output{};
    const uint32_t len = _input->payload.seqValue.len;
    for (uint32_t i = len; i > 0; i--) {
      const auto &var = _input->payload.seqValue.elements[i - 1];
      // conditional flow so we might have "returns" form (And) (Or)
      if (unlikely(_blks.activate(context, var, output, true) >
                   CBChainState::Return))
        return *_input;

      if (output == Var::True) {
        // this is acceptable cos del ops don't call free or grow
        if (_fast)
          arrayDelFast(_input->payload.seqValue, i - 1);
        else
          arrayDel(_input->payload.seqValue, i - 1);
        // remove from joined
        for (const auto &seqVar : _multiSortColumns) {
          auto &seq = seqVar->payload.seqValue;
          if (seq.elements ==
              _input->payload.seqValue
                  .elements) // avoid removing from same seq as input!
            continue;

          if (seq.len >= i) {
            if (_fast)
              arrayDelFast(seq, i - 1);
            else
              arrayDel(seq, i - 1);
          }
        }
      }
    }
    return *_input;
  }
};

struct Profile {
  BlocksVar _blocks{};
  CBExposedTypesInfo _exposed{};
  std::string _label{"<no label>"};

  static inline Parameters _params{
      {"Action", CBCCSTR("The action blocks to profile."), {CoreInfo::Blocks}},
      {"Label",
       CBCCSTR("The label to print when outputting time data."),
       {CoreInfo::StringType}}};

  static CBTypesInfo inputTypes() { return CoreInfo::AnyType; }
  static CBTypesInfo outputTypes() { return CoreInfo::AnyType; }

  static CBParametersInfo parameters() { return _params; }

  void cleanup() { _blocks.cleanup(); }

  void warmup(CBContext *ctx) { _blocks.warmup(ctx); }

  CBTypeInfo compose(const CBInstanceData &data) {
    auto res = _blocks.compose(data);
    _exposed = res.exposedInfo;
    return res.outputType;
  }

  CBExposedTypesInfo exposedVariables() { return _exposed; }

  void setParam(int index, const CBVar &value) {
    switch (index) {
    case 0:
      _blocks = value;
      break;
    case 1:
      _label = value.payload.stringValue;
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
      return Var(_label);
    default:
      break;
    }
    throw CBException("Parameter out of range.");
  }

  CBVar activate(CBContext *context, const CBVar &input) {
    CBVar output{};
    const auto start = std::chrono::high_resolution_clock::now();
    activateBlocks(CBVar(_blocks).payload.seqValue, context, input, output);
    const auto stop = std::chrono::high_resolution_clock::now();
    const auto dur =
        std::chrono::duration_cast<std::chrono::microseconds>(stop - start)
            .count();
    CBLOG_INFO("{} took {} microseconds.", dur);
    return output;
  }
};

struct XPendBase {
  static inline Types xpendTypes{{CoreInfo::AnyVarSeqType,
                                  CoreInfo::StringVarType,
                                  CoreInfo::BytesVarType}};
};

struct XpendTo : public XPendBase {
  ThreadShared<std::string> _scratchStr;

  ParamVar _collection{};

  static CBTypesInfo inputTypes() { return CoreInfo::AnyType; }
  static CBTypesInfo outputTypes() { return CoreInfo::AnyType; }

  static inline ParamsInfo paramsInfo = ParamsInfo(ParamsInfo::Param(
      "Collection", CBCCSTR("The collection to add the input to."),
      xpendTypes));

  static CBParametersInfo parameters() { return CBParametersInfo(paramsInfo); }

  CBTypeInfo compose(const CBInstanceData &data) {
    for (auto &cons : data.shared) {
      if (strcmp(cons.name, _collection.variableName()) == 0) {
        if (cons.exposedType.basicType != CBType::Seq &&
            cons.exposedType.basicType != CBType::Bytes &&
            cons.exposedType.basicType != CBType::String) {
          throw ComposeError("AppendTo/PrependTo expects either a Seq, String "
                             "or Bytes variable as collection.");
        } else {
          if (cons.exposedType.basicType != CBType::Seq &&
              cons.exposedType != data.inputType) {
            CBLOG_ERROR("Input is: {} variable is: {}", data.inputType,
                        cons.exposedType);
            throw ComposeError("Input type not matching the variable.");
          }
        }
        if (!cons.isMutable) {
          throw ComposeError(
              "AppendTo/PrependTo expects a mutable variable (Set/Push).");
        }
        if (cons.exposedType.basicType == Seq &&
            (cons.exposedType.seqTypes.len != 1 ||
             cons.exposedType.seqTypes.elements[0] != data.inputType)) {
          throw ComposeError("AppendTo/PrependTo input type is not compatible "
                             "with the backing Seq.");
        }
        // Validation Ok if here..
        return data.inputType;
      }
    }
    throw ComposeError("AppendTo/PrependTo: Failed to find variable: " +
                       std::string(_collection.variableName()));
  }

  void setParam(int index, const CBVar &value) {
    switch (index) {
    case 0:
      _collection = value;
      break;
    default:
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
      return _collection;
    default:
      break;
    }
    throw CBException("Parameter out of range.");
  }

  void cleanup() { _collection.cleanup(); }
  void warmup(CBContext *context) { _collection.warmup(context); }
};

struct AppendTo : public XpendTo {
  CBVar activate(CBContext *context, const CBVar &input) {
    auto &collection = _collection.get();
    switch (collection.valueType) {
    case Seq: {
      auto &arr = collection.payload.seqValue;
      const auto len = arr.len;
      chainblocks::arrayResize(arr, len + 1);
      cloneVar(arr.elements[len], input);
      break;
    }
    case String: {
      // variable is mutable, so we are sure we manage the memory
      // specifically in Set, cloneVar is used, which uses `new` to allocate
      // all we have to do use to clone our scratch on top of it
      _scratchStr().clear();
      _scratchStr().append(collection.payload.stringValue,
                           CBSTRLEN(collection));
      _scratchStr().append(input.payload.stringValue, CBSTRLEN(input));
      Var tmp(_scratchStr());
      cloneVar(collection, tmp);
      break;
    }
    case Bytes: {
      // we know it's a mutable variable so must be compatible with our
      // arrayPush and such routines just do like string for now basically
      _scratchStr().clear();
      _scratchStr().append((char *)collection.payload.bytesValue,
                           collection.payload.bytesSize);
      _scratchStr().append((char *)input.payload.bytesValue,
                           input.payload.bytesSize);
      Var tmp((uint8_t *)_scratchStr().data(), _scratchStr().size());
      cloneVar(collection, tmp);
    } break;
    default:
      throw ActivationError("AppendTo, case not implemented");
    }
    return input;
  }
};

struct PrependTo : public XpendTo {
  CBVar activate(CBContext *context, const CBVar &input) {
    auto &collection = _collection.get();
    switch (collection.valueType) {
    case Seq: {
      auto &arr = collection.payload.seqValue;
      const auto len = arr.len;
      chainblocks::arrayResize(arr, len + 1);
      memmove(&arr.elements[1], &arr.elements[0], sizeof(*arr.elements) * len);
      cloneVar(arr.elements[0], input);
      break;
    }
    case String: {
      // variable is mutable, so we are sure we manage the memory
      // specifically in Set, cloneVar is used, which uses `new` to allocate
      // all we have to do use to clone our scratch on top of it
      _scratchStr().clear();
      _scratchStr().append(input.payload.stringValue, CBSTRLEN(input));
      _scratchStr().append(collection.payload.stringValue,
                           CBSTRLEN(collection));
      Var tmp(_scratchStr());
      cloneVar(collection, tmp);
      break;
    }
    case Bytes: {
      // we know it's a mutable variable so must be compatible with our
      // arrayPush and such routines just do like string for now basically
      _scratchStr().clear();
      _scratchStr().append((char *)input.payload.bytesValue,
                           input.payload.bytesSize);
      _scratchStr().append((char *)collection.payload.bytesValue,
                           collection.payload.bytesSize);
      Var tmp((uint8_t *)_scratchStr().data(), _scratchStr().size());
      cloneVar(collection, tmp);
    } break;
    default:
      throw ActivationError("PrependTo, case not implemented");
    }
    return input;
  }
};

struct ForEachBlock {
  static inline Types _types{{CoreInfo::AnySeqType, CoreInfo::AnyTableType}};

  static CBTypesInfo inputTypes() { return _types; }

  static CBTypesInfo outputTypes() { return _types; }

  static CBParametersInfo parameters() { return _params; }

  void setParam(int index, const CBVar &value) { _blocks = value; }

  CBVar getParam(int index) { return _blocks; }

  CBTypeInfo compose(const CBInstanceData &data) {
    if (data.inputType.basicType != Seq && data.inputType.basicType != Table) {
      throw ComposeError(
          "ForEach block expected a sequence or a table as input.");
    }

    auto dataCopy = data;
    if (data.inputType.basicType == Seq && data.inputType.seqTypes.len == 1) {
      dataCopy.inputType = data.inputType.seqTypes.elements[0];
    } else if (data.inputType.basicType == Table) {
      dataCopy.inputType = CoreInfo::AnySeqType;
    } else {
      dataCopy.inputType = CoreInfo::AnyType;
    }

    _blocks.compose(dataCopy);

    if (data.inputType.basicType == Table) {
      OVERRIDE_ACTIVATE(data, activateTable);
    } else {
      OVERRIDE_ACTIVATE(data, activateSeq);
    }

    return data.inputType;
  }

  void warmup(CBContext *ctx) { _blocks.warmup(ctx); }

  void cleanup() { _blocks.cleanup(); }

  CBVar activateSeq(CBContext *context, const CBVar &input) {
    CBVar output{};
    for (auto &item : input) {
      auto state = _blocks.activate(context, item, output, true);
      // handle return short circuit, assume it was for us
      if (state != CBChainState::Continue)
        break;
    }
    return input;
  }

  std::array<OwnedVar, 2> _tableItem;

  CBVar activateTable(CBContext *context, const CBVar &input) {
    const auto &table = input.payload.tableValue;
    CBVar output{};
    ForEach(table, [&](auto key, auto &val) {
      _tableItem[0] = Var(key);
      _tableItem[1] = val;
      auto &itemref = _tableItem;
      const auto item = Var(reinterpret_cast<std::array<CBVar, 2> &>(itemref));
      const auto state = _blocks.activate(context, item, output, true);
      // handle return short circuit, assume it was for us
      if (unlikely(state != CBChainState::Continue))
        return false;
      else
        return true;
    });
    return input;
  }

  CBVar activate(CBContext *context, const CBVar &input) {
    throw ActivationError("Invalid activation path");
    return input;
  }

private:
  static inline Parameters _params{
      {"Apply",
       CBCCSTR("The function to apply to each item of the sequence."),
       {CoreInfo::Blocks}}};

  BlocksVar _blocks{};
};

struct Map {
  CBTypesInfo inputTypes() { return CoreInfo::AnySeqType; }

  CBTypesInfo outputTypes() { return CoreInfo::AnySeqType; }

  CBParametersInfo parameters() { return _params; }

  void setParam(int index, const CBVar &value) { _blocks = value; }

  CBVar getParam(int index) { return _blocks; }

  void destroy() { destroyVar(_output); }

  CBTypeInfo compose(const CBInstanceData &data) {
    if (data.inputType.seqTypes.len != 1) {
      throw CBException(
          "Map: Invalid sequence inner type, must be a single defined type.");
    }
    CBInstanceData dataCopy = data;
    dataCopy.inputType = data.inputType.seqTypes.elements[0];
    auto innerRes = _blocks.compose(dataCopy);
    _outputSingleType = innerRes.outputType;
    _outputType = {CBType::Seq, {.seqTypes = {&_outputSingleType, 1, 0}}};
    return _outputType;
  }

  void warmup(CBContext *ctx) {
    _output.valueType = Seq;
    _blocks.warmup(ctx);
  }

  void cleanup() { _blocks.cleanup(); }

  CBVar activate(CBContext *context, const CBVar &input) {
    CBVar output{};
    arrayResize(_output.payload.seqValue, 0);
    for (auto &item : input) {
      // handle return short circuit, assume it was for us
      auto state = _blocks.activate(context, item, output, true);
      if (state != CBChainState::Continue)
        break;
      arrayPush(_output.payload.seqValue, output);
    }
    return _output;
  }

private:
  static inline Parameters _params{
      {"Apply",
       CBCCSTR("The function to apply to each item of the sequence."),
       {CoreInfo::Blocks}}};

  CBVar _output{};
  BlocksVar _blocks{};
  CBTypeInfo _outputSingleType{};
  Type _outputType{};
};

struct Reduce {
  CBTypesInfo inputTypes() { return CoreInfo::AnySeqType; }

  CBTypesInfo outputTypes() { return CoreInfo::AnyType; }

  CBParametersInfo parameters() { return _params; }

  void setParam(int index, const CBVar &value) { _blocks = value; }

  CBVar getParam(int index) { return _blocks; }

  void destroy() { destroyVar(_output); }

  CBTypeInfo compose(const CBInstanceData &data) {
    if (data.inputType.seqTypes.len != 1) {
      throw CBException("Reduce: Invalid sequence inner type, must be a single "
                        "defined type.");
    }
    // we need to edit a copy of data
    CBInstanceData dataCopy = data;
    // we need to deep copy it
    dataCopy.shared = {};
    DEFER({ arrayFree(dataCopy.shared); });
    dataCopy.inputType = data.inputType.seqTypes.elements[0];
    // copy killing any existing $0
    for (uint32_t i = data.shared.len; i > 0; i--) {
      auto idx = i - 1;
      auto &item = data.shared.elements[idx];
      if (strcmp(item.name, "$0") != 0) {
        arrayPush(dataCopy.shared, item);
      }
    }
    _tmpInfo.exposedType = dataCopy.inputType;
    arrayPush(dataCopy.shared, _tmpInfo);
    auto innerRes = _blocks.compose(dataCopy);
    _outputSingleType = innerRes.outputType;
    return _outputSingleType;
  }

  void warmup(CBContext *ctx) {
    _tmp = referenceVariable(ctx, "$0");
    _blocks.warmup(ctx);
  }

  void cleanup() {
    _blocks.cleanup();
    releaseVariable(_tmp);
    _tmp = nullptr;
  }

  CBVar activate(CBContext *context, const CBVar &input) {
    if (input.payload.seqValue.len == 0) {
      throw ActivationError("Reduce: Input sequence was empty!");
    }
    cloneVar(*_tmp, input.payload.seqValue.elements[0]);
    CBVar output{};
    for (uint32_t i = 1; i < input.payload.seqValue.len; i++) {
      auto &item = input.payload.seqValue.elements[i];
      // allow short circut with (Return)
      auto state = _blocks.activate(context, item, output, true);
      if (state != CBChainState::Continue)
        break;
      cloneVar(*_tmp, output);
    }
    cloneVar(_output, *_tmp);
    return _output;
  }

private:
  static inline Parameters _params{
      {"Apply",
       CBCCSTR("The function to apply to each item of the sequence."),
       {CoreInfo::Blocks}}};

  CBVar *_tmp = nullptr;
  CBVar _output{};
  BlocksVar _blocks{};
  CBTypeInfo _outputSingleType{};
  CBExposedTypeInfo _tmpInfo{"$0"};
};

struct Erase : SeqUser {
  static CBTypesInfo inputTypes() { return CoreInfo::AnyType; }

  static CBTypesInfo outputTypes() { return CoreInfo::AnyType; }

  static CBParametersInfo parameters() { return _params; }

  void warmup(CBContext *ctx) {
    SeqUser::warmup(ctx);
    _indices.warmup(ctx);
  }

  void cleanup() {
    _indices.cleanup();
    SeqUser::cleanup();
  }

  void setParam(int index, const CBVar &value) {
    if (index == 0) {
      _indices = value;
    } else {
      index--;
      SeqUser::setParam(index, value);
    }
  }

  CBVar getParam(int index) {
    if (index == 0) {
      return _indices;
    } else {
      index--;
      return SeqUser::getParam(index);
    }
  }

  CBTypeInfo compose(const CBInstanceData &data) {
    bool valid = false;
    _isTable = data.inputType.basicType == Table;
    // Figure if we output a sequence or not
    if (_indices->valueType == Seq) {
      if (_indices->payload.seqValue.len > 0) {
        if ((_indices->payload.seqValue.elements[0].valueType == Int &&
             !_isTable) ||
            (_indices->payload.seqValue.elements[0].valueType == String &&
             _isTable)) {
          valid = true;
        }
      }
    } else if ((!_isTable && _indices->valueType == Int) ||
               (_isTable && _indices->valueType == String)) {
      valid = true;
    } else { // ContextVar
      for (auto &info : data.shared) {
        if (strcmp(info.name, _indices->payload.stringValue) == 0) {
          if (info.exposedType.basicType == Seq &&
              info.exposedType.seqTypes.len == 1 &&
              ((info.exposedType.seqTypes.elements[0].basicType == Int &&
                !_isTable) ||
               (info.exposedType.seqTypes.elements[0].basicType == String &&
                _isTable))) {
            valid = true;
            break;
          } else if (info.exposedType.basicType == Int && !_isTable) {
            valid = true;
            break;
          } else if (info.exposedType.basicType == String && _isTable) {
            valid = true;
            break;
          } else {
            auto msg = "Take indices variable " + std::string(info.name) +
                       " expected to be either Seq, Int or String";
            throw CBException(msg);
          }
        }
      }
    }

    if (!valid)
      throw CBException("Erase, invalid indices or malformed input.");
    return data.inputType;
  }

  CBVar activate(CBContext *context, const CBVar &input) {
    const auto &indices = _indices.get();
    if (unlikely(_isTable && _target->valueType == Table)) {
      if (indices.valueType == String) {
        // single key
        const auto key = indices.payload.stringValue;
        _target->payload.tableValue.api->tableRemove(
            _target->payload.tableValue, key);
      } else {
        // multiple keys
        const uint32_t nkeys = indices.payload.seqValue.len;
        for (uint32_t i = 0; nkeys > i; i++) {
          const auto key =
              indices.payload.seqValue.elements[i].payload.stringValue;
          _target->payload.tableValue.api->tableRemove(
              _target->payload.tableValue, key);
        }
      }
    } else {
      if (indices.valueType == Int) {
        const auto index = indices.payload.intValue;
        arrayDel(_target->payload.seqValue, index);
      } else {
        // ensure we delete from highest index
        // so to keep indices always valid
        IterableSeq sindices(indices);
        pdqsort(sindices.begin(), sindices.end(),
                [](CBVar a, CBVar b) { return a > b; });
        for (auto &idx : sindices) {
          const auto index = idx.payload.intValue;
          arrayDel(_target->payload.seqValue, index);
        }
      }
    }
    return input;
  }

private:
  ParamVar _indices{};
  static inline Parameters _params = {
      {"Indices", CBCCSTR("One or multiple indices to filter from a sequence."),
       CoreInfo::TakeTypes},
      {"Name", CBCCSTR("The name of the variable."), CoreInfo::StringOrAnyVar},
      {"Key",
       CBCCSTR("The key of the value to read/write from/in the table (this "
               "variable will become a table)."),
       {CoreInfo::StringType}},
      {"Global",
       CBCCSTR("If the variable is or should be available to all of the chains "
               "in the same node."),
       {CoreInfo::BoolType}}};
  bool _isTable;
};

struct Assoc : public VariableBase {
  ExposedInfo _requiredInfo{};
  Type _tableTypes{};

  static CBTypesInfo inputTypes() { return CoreInfo::AnySeqType; }
  static CBTypesInfo outputTypes() { return CoreInfo::AnySeqType; }

  CBExposedTypesInfo requiredVariables() {
    _requiredInfo = ExposedInfo(ExposedInfo::Variable(
        _name.c_str(), CBCCSTR("The required variable."), CoreInfo::AnyType));
    return CBExposedTypesInfo(_requiredInfo);
  }

  // TODO we need to evaluate deeper and figure out we don't mutate types

  // CBTypeInfo compose(const CBInstanceData &data) {
  //   _exposedInfo = {};

  //   if (_isTable) {
  //     _tableTypes = Type::TableOf(data.inputType.seqTypes);
  //     if (_global) {
  //       _exposedInfo = ExposedInfo(ExposedInfo::GlobalVariable(
  //           _name.c_str(), CBCCSTR("The exposed table."), _tableTypes,
  //           true));
  //     } else {
  //       _exposedInfo = ExposedInfo(ExposedInfo::Variable(
  //           _name.c_str(), CBCCSTR("The exposed table."), _tableTypes,
  //           true));
  //     }
  //   } else {
  //     if (_global) {
  //       _exposedInfo = ExposedInfo(ExposedInfo::GlobalVariable(
  //           _name.c_str(), CBCCSTR("The exposed sequence."), data.inputType,
  //           true));
  //     } else {
  //       _exposedInfo = ExposedInfo(ExposedInfo::Variable(
  //           _name.c_str(), CBCCSTR("The exposed sequence."), data.inputType,
  //           true));
  //     }
  //   }

  //   return data.inputType;
  // }

  // CBExposedTypesInfo exposedVariables() {
  //   return CBExposedTypesInfo(_exposedInfo);
  // }

  void warmup(CBContext *context) {
    if (_global)
      _target = referenceGlobalVariable(context, _name.c_str());
    else
      _target = referenceVariable(context, _name.c_str());
    if (_serialized)
      _target->flags |= CBVAR_FLAGS_SHOULD_SERIALIZE;
    _key.warmup(context);
  }

  CBVar activate(CBContext *context, const CBVar &input) {
    if (likely(_cell != nullptr)) {
      if ((input.payload.seqValue.len % 2) != 0) {
        throw ActivationError("Expected an even sized sequence as input.");
      }

      auto n = input.payload.seqValue.len / 2;

      if (_cell->valueType == Seq) {
        auto &s = _cell->payload.seqValue;
        for (uint32_t i = 0; i < n; i++) {
          auto &idx = input.payload.seqValue.elements[(i * 2) + 0];
          if (idx.valueType != Int) {
            throw ActivationError("Expected an Int for index.");
          }
          auto index = uint32_t(idx.payload.intValue);
          if (index >= s.len) {
            throw ActivationError("Index out of range, sequence is smaller.");
          }
          auto &v = input.payload.seqValue.elements[(i * 2) + 1];
          cloneVar(s.elements[index], v);
        }
      } else if (_cell->valueType == Table) {
        auto &t = _cell->payload.tableValue;
        for (uint32_t i = 0; i < n; i++) {
          auto &k = input.payload.seqValue.elements[(i * 2) + 0];
          if (k.valueType != String) {
            throw ActivationError("Expected a String for key.");
          }
          auto &v = input.payload.seqValue.elements[(i * 2) + 1];
          CBVar *record = t.api->tableAt(t, k.payload.stringValue);
          cloneVar(*record, v);
        }
      } else {
        throw ActivationError("Expected table or sequence variable.");
      }

      if (_isTable && _key.isVariable())
        _cell = nullptr; // need to cleanup as it's a variable

      return input;
    } else {
      if (_isTable) {
        if (_target->valueType == Table) {
          auto &kv = _key.get();
          if (_target->payload.tableValue.api->tableContains(
                  _target->payload.tableValue, kv.payload.stringValue)) {
            // Has it
            CBVar *vptr = _target->payload.tableValue.api->tableAt(
                _target->payload.tableValue, kv.payload.stringValue);
            // Pin fast cell
            _cell = vptr;
          } else {
            throw ActivationError("Key not found in table.");
          }
        } else {
          throw ActivationError("Table is empty or does not exist yet.");
        }
      } else {
        if (_target->valueType == Seq || _target->valueType == Table) {
          // Pin fast cell
          _cell = _target;
        } else {
          throw ActivationError("Variable is empty or does not exist yet.");
        }
      }
      // recurse in, now that we have cell
      assert(_cell);
      return activate(context, input);
    }
  }
};

struct Replace {
  static inline Types inTypes{{CoreInfo::AnySeqType, CoreInfo::StringType}};
  static inline Parameters params{
      {"Patterns",
       CBCCSTR("The patterns to find."),
       {CoreInfo::NoneType, CoreInfo::StringSeqType, CoreInfo::StringVarSeqType,
        CoreInfo::AnyVarSeqType, CoreInfo::AnySeqType}},
      {"Replacements",
       CBCCSTR("The replacements to apply to the input, if a single value is "
               "provided every match will be replaced with that single value."),
       {CoreInfo::NoneType, CoreInfo::AnyType, CoreInfo::AnyVarType,
        CoreInfo::AnySeqType, CoreInfo::AnyVarSeqType}}};

  static CBTypesInfo inputTypes() { return inTypes; }
  static CBTypesInfo outputTypes() { return inTypes; }
  static CBParametersInfo parameters() { return params; }

  ParamVar _patterns{};
  ParamVar _replacements{};
  std::string _stringOutput;
  OwnedVar _vectorOutput;

  void setParam(int index, const CBVar &value) {
    switch (index) {
    case 0:
      _patterns = value;
      break;
    case 1:
      _replacements = value;
      break;
    default:
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
      return _patterns;
    case 1:
      return _replacements;
    default:
      return Var::Empty;
    }
  }

  CBTypeInfo compose(const CBInstanceData &data) {
    if (_patterns->valueType == None) {
      data.block->inlineBlockId = NoopBlock;
    } else {
      data.block->inlineBlockId = NotInline;
    }

    if (data.inputType.basicType == String) {
      OVERRIDE_ACTIVATE(data, activateString);
      return CoreInfo::StringType;
    } else {
      // this should be only sequence
      OVERRIDE_ACTIVATE(data, activateSeq);
      return data.inputType;
    }
  }

  void warmup(CBContext *context) {
    _patterns.warmup(context);
    _replacements.warmup(context);
  }

  void cleanup() {
    _patterns.cleanup();
    _replacements.cleanup();
  }

  CBVar activateSeq(CBContext *context, const CBVar &input) {
    _vectorOutput = input;
    IterableSeq o(_vectorOutput);
    const auto &patterns = _patterns.get();
    const auto &replacements = _replacements.get();
    if (replacements.valueType == Seq) {
      if (patterns.payload.seqValue.len != replacements.payload.seqValue.len) {
        throw ActivationError("Translate patterns size mismatch, must be equal "
                              "to replacements size.");
      }

      for (uint32_t i = 0; i < patterns.payload.seqValue.len; i++) {
        const auto &p = patterns.payload.seqValue.elements[i];
        const auto &r = replacements.payload.seqValue.elements[i];
        std::replace(o.begin(), o.end(), p, r);
      }
    } else {
      for (uint32_t i = 0; i < patterns.payload.seqValue.len; i++) {
        const auto &p = patterns.payload.seqValue.elements[i];
        std::replace(o.begin(), o.end(), p, replacements);
      }
    }
    return Var(_vectorOutput);
  }

  CBVar activateString(CBContext *context, const CBVar &input) {
    const auto source = input.payload.stringLen > 0
                            ? std::string_view(input.payload.stringValue,
                                               input.payload.stringLen)
                            : std::string_view(input.payload.stringValue);
    _stringOutput.assign(source);
    const auto &patterns = _patterns.get();
    const auto &replacements = _replacements.get();
    if (replacements.valueType == Seq) {
      if (patterns.payload.seqValue.len != replacements.payload.seqValue.len) {
        throw ActivationError("Translate patterns size mismatch, must be equal "
                              "to replacements size.");
      }
      for (uint32_t i = 0; i < patterns.payload.seqValue.len; i++) {
        const auto &p = patterns.payload.seqValue.elements[i];
        const auto &r = replacements.payload.seqValue.elements[i];
        const auto pattern =
            p.payload.stringLen > 0
                ? std::string_view(p.payload.stringValue, p.payload.stringLen)
                : std::string_view(p.payload.stringValue);
        const auto replacement =
            r.payload.stringLen > 0
                ? std::string_view(r.payload.stringValue, r.payload.stringLen)
                : std::string_view(r.payload.stringValue);
        boost::replace_all(_stringOutput, pattern, replacement);
      }
    } else {
      const auto replacement =
          replacements.payload.stringLen > 0
              ? std::string_view(replacements.payload.stringValue,
                                 replacements.payload.stringLen)
              : std::string_view(replacements.payload.stringValue);
      for (uint32_t i = 0; i < patterns.payload.seqValue.len; i++) {
        const auto &p = patterns.payload.seqValue.elements[i];
        const auto pattern =
            p.payload.stringLen > 0
                ? std::string_view(p.payload.stringValue, p.payload.stringLen)
                : std::string_view(p.payload.stringValue);

        boost::replace_all(_stringOutput, pattern, replacement);
      }
    }
    return Var(_stringOutput);
  }

  CBVar activate(CBContext *context, const CBVar &input) {
    throw ActivationError("Invalid activation function");
  }
};

// Register And
RUNTIME_CORE_BLOCK_FACTORY(And);
RUNTIME_BLOCK_inputTypes(And);
RUNTIME_BLOCK_outputTypes(And);
RUNTIME_BLOCK_activate(And);
RUNTIME_BLOCK_END(And);

// Register Or
RUNTIME_CORE_BLOCK_FACTORY(Or);
RUNTIME_BLOCK_inputTypes(Or);
RUNTIME_BLOCK_outputTypes(Or);
RUNTIME_BLOCK_activate(Or);
RUNTIME_BLOCK_END(Or);

// Register Not
RUNTIME_CORE_BLOCK_FACTORY(Not);
RUNTIME_BLOCK_inputTypes(Not);
RUNTIME_BLOCK_outputTypes(Not);
RUNTIME_BLOCK_activate(Not);
RUNTIME_BLOCK_END(Not);

// Register IsNan
RUNTIME_CORE_BLOCK_FACTORY(IsValidNumber);
RUNTIME_BLOCK_inputTypes(IsValidNumber);
RUNTIME_BLOCK_outputTypes(IsValidNumber);
RUNTIME_BLOCK_activate(IsValidNumber);
RUNTIME_BLOCK_END(IsValidNumber);

// Register Set
RUNTIME_CORE_BLOCK_FACTORY(Set);
RUNTIME_BLOCK_cleanup(Set);
RUNTIME_BLOCK_warmup(Set);
RUNTIME_BLOCK_inputTypes(Set);
RUNTIME_BLOCK_outputTypes(Set);
RUNTIME_BLOCK_parameters(Set);
RUNTIME_BLOCK_compose(Set);
RUNTIME_BLOCK_exposedVariables(Set);
RUNTIME_BLOCK_setParam(Set);
RUNTIME_BLOCK_getParam(Set);
RUNTIME_BLOCK_activate(Set);
RUNTIME_BLOCK_END(Set);

// Register Ref
RUNTIME_CORE_BLOCK_FACTORY(Ref);
RUNTIME_BLOCK_cleanup(Ref);
RUNTIME_BLOCK_warmup(Ref);
RUNTIME_BLOCK_inputTypes(Ref);
RUNTIME_BLOCK_outputTypes(Ref);
RUNTIME_BLOCK_parameters(Ref);
RUNTIME_BLOCK_compose(Ref);
RUNTIME_BLOCK_exposedVariables(Ref);
RUNTIME_BLOCK_setParam(Ref);
RUNTIME_BLOCK_getParam(Ref);
RUNTIME_BLOCK_activate(Ref);
RUNTIME_BLOCK_END(Ref);

// Register Update
RUNTIME_CORE_BLOCK_FACTORY(Update);
RUNTIME_BLOCK_cleanup(Update);
RUNTIME_BLOCK_warmup(Update);
RUNTIME_BLOCK_inputTypes(Update);
RUNTIME_BLOCK_outputTypes(Update);
RUNTIME_BLOCK_parameters(Update);
RUNTIME_BLOCK_compose(Update);
RUNTIME_BLOCK_requiredVariables(Update);
RUNTIME_BLOCK_setParam(Update);
RUNTIME_BLOCK_getParam(Update);
RUNTIME_BLOCK_activate(Update);
RUNTIME_BLOCK_END(Update);

// Register Push
RUNTIME_CORE_BLOCK_FACTORY(Push);
RUNTIME_BLOCK_cleanup(Push);
RUNTIME_BLOCK_destroy(Push);
RUNTIME_BLOCK_warmup(Push);
RUNTIME_BLOCK_inputTypes(Push);
RUNTIME_BLOCK_outputTypes(Push);
RUNTIME_BLOCK_parameters(Push);
RUNTIME_BLOCK_compose(Push);
RUNTIME_BLOCK_exposedVariables(Push);
RUNTIME_BLOCK_setParam(Push);
RUNTIME_BLOCK_getParam(Push);
RUNTIME_BLOCK_activate(Push);
RUNTIME_BLOCK_END(Push);

// Register Sequence
RUNTIME_CORE_BLOCK_FACTORY(Sequence);
RUNTIME_BLOCK_cleanup(Sequence);
RUNTIME_BLOCK_destroy(Sequence);
RUNTIME_BLOCK_warmup(Sequence);
RUNTIME_BLOCK_inputTypes(Sequence);
RUNTIME_BLOCK_outputTypes(Sequence);
RUNTIME_BLOCK_parameters(Sequence);
RUNTIME_BLOCK_compose(Sequence);
RUNTIME_BLOCK_exposedVariables(Sequence);
RUNTIME_BLOCK_setParam(Sequence);
RUNTIME_BLOCK_getParam(Sequence);
RUNTIME_BLOCK_activate(Sequence);
RUNTIME_BLOCK_END(Sequence);

// Register TableDecl
RUNTIME_CORE_BLOCK_FACTORY(TableDecl);
RUNTIME_BLOCK_cleanup(TableDecl);
RUNTIME_BLOCK_destroy(TableDecl);
RUNTIME_BLOCK_warmup(TableDecl);
RUNTIME_BLOCK_inputTypes(TableDecl);
RUNTIME_BLOCK_outputTypes(TableDecl);
RUNTIME_BLOCK_parameters(TableDecl);
RUNTIME_BLOCK_compose(TableDecl);
RUNTIME_BLOCK_exposedVariables(TableDecl);
RUNTIME_BLOCK_setParam(TableDecl);
RUNTIME_BLOCK_getParam(TableDecl);
RUNTIME_BLOCK_activate(TableDecl);
RUNTIME_BLOCK_END(TableDecl);

// Register Pop
RUNTIME_CORE_BLOCK_FACTORY(Pop);
RUNTIME_BLOCK_cleanup(Pop);
RUNTIME_BLOCK_warmup(Pop);
RUNTIME_BLOCK_destroy(Pop);
RUNTIME_BLOCK_inputTypes(Pop);
RUNTIME_BLOCK_outputTypes(Pop);
RUNTIME_BLOCK_parameters(Pop);
RUNTIME_BLOCK_compose(Pop);
RUNTIME_BLOCK_requiredVariables(Pop);
RUNTIME_BLOCK_setParam(Pop);
RUNTIME_BLOCK_getParam(Pop);
RUNTIME_BLOCK_activate(Pop);
RUNTIME_BLOCK_END(Pop);

// Register PopFront
RUNTIME_CORE_BLOCK_FACTORY(PopFront);
RUNTIME_BLOCK_cleanup(PopFront);
RUNTIME_BLOCK_warmup(PopFront);
RUNTIME_BLOCK_destroy(PopFront);
RUNTIME_BLOCK_inputTypes(PopFront);
RUNTIME_BLOCK_outputTypes(PopFront);
RUNTIME_BLOCK_parameters(PopFront);
RUNTIME_BLOCK_compose(PopFront);
RUNTIME_BLOCK_requiredVariables(PopFront);
RUNTIME_BLOCK_setParam(PopFront);
RUNTIME_BLOCK_getParam(PopFront);
RUNTIME_BLOCK_activate(PopFront);
RUNTIME_BLOCK_END(PopFront);

// Register Count
RUNTIME_CORE_BLOCK_FACTORY(Count);
RUNTIME_BLOCK_cleanup(Count);
RUNTIME_BLOCK_warmup(Count);
RUNTIME_BLOCK_inputTypes(Count);
RUNTIME_BLOCK_outputTypes(Count);
RUNTIME_BLOCK_parameters(Count);
RUNTIME_BLOCK_setParam(Count);
RUNTIME_BLOCK_getParam(Count);
RUNTIME_BLOCK_activate(Count);
RUNTIME_BLOCK_END(Count);

// Register Clear
RUNTIME_CORE_BLOCK_FACTORY(Clear);
RUNTIME_BLOCK_cleanup(Clear);
RUNTIME_BLOCK_warmup(Clear);
RUNTIME_BLOCK_inputTypes(Clear);
RUNTIME_BLOCK_outputTypes(Clear);
RUNTIME_BLOCK_parameters(Clear);
RUNTIME_BLOCK_setParam(Clear);
RUNTIME_BLOCK_getParam(Clear);
RUNTIME_BLOCK_activate(Clear);
RUNTIME_BLOCK_END(Clear);

// Register Drop
RUNTIME_CORE_BLOCK_FACTORY(Drop);
RUNTIME_BLOCK_cleanup(Drop);
RUNTIME_BLOCK_warmup(Drop);
RUNTIME_BLOCK_inputTypes(Drop);
RUNTIME_BLOCK_outputTypes(Drop);
RUNTIME_BLOCK_parameters(Drop);
RUNTIME_BLOCK_setParam(Drop);
RUNTIME_BLOCK_getParam(Drop);
RUNTIME_BLOCK_activate(Drop);
RUNTIME_BLOCK_END(Drop);

// Register DropFront
RUNTIME_CORE_BLOCK_FACTORY(DropFront);
RUNTIME_BLOCK_cleanup(DropFront);
RUNTIME_BLOCK_warmup(DropFront);
RUNTIME_BLOCK_inputTypes(DropFront);
RUNTIME_BLOCK_outputTypes(DropFront);
RUNTIME_BLOCK_parameters(DropFront);
RUNTIME_BLOCK_setParam(DropFront);
RUNTIME_BLOCK_getParam(DropFront);
RUNTIME_BLOCK_activate(DropFront);
RUNTIME_BLOCK_END(DropFront);

// Register Get
RUNTIME_CORE_BLOCK_FACTORY(Get);
RUNTIME_BLOCK_cleanup(Get);
RUNTIME_BLOCK_warmup(Get);
RUNTIME_BLOCK_destroy(Get);
RUNTIME_BLOCK_inputTypes(Get);
RUNTIME_BLOCK_outputTypes(Get);
RUNTIME_BLOCK_parameters(Get);
RUNTIME_BLOCK_compose(Get);
RUNTIME_BLOCK_requiredVariables(Get);
RUNTIME_BLOCK_setParam(Get);
RUNTIME_BLOCK_getParam(Get);
RUNTIME_BLOCK_activate(Get);
RUNTIME_BLOCK_END(Get);

// Register Swap
RUNTIME_CORE_BLOCK_FACTORY(Swap);
RUNTIME_BLOCK_inputTypes(Swap);
RUNTIME_BLOCK_warmup(Swap);
RUNTIME_BLOCK_cleanup(Swap);
RUNTIME_BLOCK_outputTypes(Swap);
RUNTIME_BLOCK_parameters(Swap);
RUNTIME_BLOCK_requiredVariables(Swap);
RUNTIME_BLOCK_setParam(Swap);
RUNTIME_BLOCK_getParam(Swap);
RUNTIME_BLOCK_activate(Swap);
RUNTIME_BLOCK_END(Swap);

// Register Take
RUNTIME_CORE_BLOCK_FACTORY(Take);
RUNTIME_BLOCK_destroy(Take);
RUNTIME_BLOCK_cleanup(Take);
RUNTIME_BLOCK_requiredVariables(Take);
RUNTIME_BLOCK_inputTypes(Take);
RUNTIME_BLOCK_outputTypes(Take);
RUNTIME_BLOCK_parameters(Take);
RUNTIME_BLOCK_compose(Take);
RUNTIME_BLOCK_setParam(Take);
RUNTIME_BLOCK_getParam(Take);
RUNTIME_BLOCK_warmup(Take);
RUNTIME_BLOCK_activate(Take);
RUNTIME_BLOCK_END(Take);

// Register RTake
RUNTIME_CORE_BLOCK_FACTORY(RTake);
RUNTIME_BLOCK_destroy(RTake);
RUNTIME_BLOCK_cleanup(RTake);
RUNTIME_BLOCK_requiredVariables(RTake);
RUNTIME_BLOCK_inputTypes(RTake);
RUNTIME_BLOCK_outputTypes(RTake);
RUNTIME_BLOCK_parameters(RTake);
RUNTIME_BLOCK_compose(RTake);
RUNTIME_BLOCK_setParam(RTake);
RUNTIME_BLOCK_getParam(RTake);
RUNTIME_BLOCK_warmup(RTake);
RUNTIME_BLOCK_activate(RTake);
RUNTIME_BLOCK_END(RTake);

// Register Slice
RUNTIME_CORE_BLOCK_FACTORY(Slice);
RUNTIME_BLOCK_destroy(Slice);
RUNTIME_BLOCK_cleanup(Slice);
RUNTIME_BLOCK_requiredVariables(Slice);
RUNTIME_BLOCK_inputTypes(Slice);
RUNTIME_BLOCK_outputTypes(Slice);
RUNTIME_BLOCK_parameters(Slice);
RUNTIME_BLOCK_compose(Slice);
RUNTIME_BLOCK_setParam(Slice);
RUNTIME_BLOCK_getParam(Slice);
RUNTIME_BLOCK_activate(Slice);
RUNTIME_BLOCK_END(Slice);

// Register Limit
RUNTIME_CORE_BLOCK_FACTORY(Limit);
RUNTIME_BLOCK_destroy(Limit);
RUNTIME_BLOCK_inputTypes(Limit);
RUNTIME_BLOCK_outputTypes(Limit);
RUNTIME_BLOCK_parameters(Limit);
RUNTIME_BLOCK_compose(Limit);
RUNTIME_BLOCK_setParam(Limit);
RUNTIME_BLOCK_getParam(Limit);
RUNTIME_BLOCK_activate(Limit);
RUNTIME_BLOCK_END(Limit);

// Register Repeat
RUNTIME_CORE_BLOCK_FACTORY(Repeat);
RUNTIME_BLOCK_inputTypes(Repeat);
RUNTIME_BLOCK_outputTypes(Repeat);
RUNTIME_BLOCK_parameters(Repeat);
RUNTIME_BLOCK_setParam(Repeat);
RUNTIME_BLOCK_getParam(Repeat);
RUNTIME_BLOCK_activate(Repeat);
RUNTIME_BLOCK_cleanup(Repeat);
RUNTIME_BLOCK_warmup(Repeat);
RUNTIME_BLOCK_requiredVariables(Repeat);
RUNTIME_BLOCK_compose(Repeat);
RUNTIME_BLOCK_END(Repeat);

// Register Sort
RUNTIME_CORE_BLOCK(Sort);
RUNTIME_BLOCK_setup(Sort);
RUNTIME_BLOCK_inputTypes(Sort);
RUNTIME_BLOCK_outputTypes(Sort);
RUNTIME_BLOCK_compose(Sort);
RUNTIME_BLOCK_activate(Sort);
RUNTIME_BLOCK_parameters(Sort);
RUNTIME_BLOCK_setParam(Sort);
RUNTIME_BLOCK_getParam(Sort);
RUNTIME_BLOCK_cleanup(Sort);
RUNTIME_BLOCK_warmup(Sort);
RUNTIME_BLOCK_END(Sort);

// Register
RUNTIME_CORE_BLOCK(Remove);
RUNTIME_BLOCK_inputTypes(Remove);
RUNTIME_BLOCK_outputTypes(Remove);
RUNTIME_BLOCK_parameters(Remove);
RUNTIME_BLOCK_setParam(Remove);
RUNTIME_BLOCK_getParam(Remove);
RUNTIME_BLOCK_activate(Remove);
RUNTIME_BLOCK_cleanup(Remove);
RUNTIME_BLOCK_warmup(Remove);
RUNTIME_BLOCK_compose(Remove);
RUNTIME_BLOCK_END(Remove);

LOGIC_OP_DESC(Is);
LOGIC_OP_DESC(IsNot);
LOGIC_OP_DESC(IsMore);
LOGIC_OP_DESC(IsLess);
LOGIC_OP_DESC(IsMoreEqual);
LOGIC_OP_DESC(IsLessEqual);

LOGIC_OP_DESC(Any);
LOGIC_OP_DESC(All);
LOGIC_OP_DESC(AnyNot);
LOGIC_OP_DESC(AllNot);
LOGIC_OP_DESC(AnyMore);
LOGIC_OP_DESC(AllMore);
LOGIC_OP_DESC(AnyLess);
LOGIC_OP_DESC(AllLess);
LOGIC_OP_DESC(AnyMoreEqual);
LOGIC_OP_DESC(AllMoreEqual);
LOGIC_OP_DESC(AnyLessEqual);
LOGIC_OP_DESC(AllLessEqual);

CBVar unreachableActivation(const CBVar &input) { throw; }

CBVar exitProgramActivation(const CBVar &input) {
  exit(input.payload.intValue);
}

CBVar hashActivation(const CBVar &input) {
  return Var(chainblocks::hash(input));
}

CBVar blockingSleepActivation(const CBVar &input) {
  if (input.valueType == CBType::Int) {
    sleep(double(input.payload.intValue) / 1000.0, false);
  } else if (input.valueType == CBType::Float) {
    sleep(input.payload.floatValue, false);
  } else {
    throw ActivationError("Expected either Int (ms) or Float (seconds)");
  }
  return input;
}

#ifdef __EMSCRIPTEN__
extern "C" {
char *emEval(const char *code);
double now();
bool emEvalAsyncRun(const char *code, size_t index);
int emEvalAsyncCheck(size_t index);
char *emEvalAsyncGet(size_t index);
void emBrowsePage(const char *url);
}

CBVar emscriptenEvalActivation(const CBVar &input) {
  static thread_local std::string str;
  auto res = emEval(input.payload.stringValue);
  const auto check = reinterpret_cast<intptr_t>(res);
  if (check == -1) {
    throw ActivationError("Failure on the javascript side, check console");
  }
  str.clear();
  if (res) {
    str.assign(res);
    free(res);
  }
  return Var(str);
}

/*
using embind fails inside workers... that's why we implemented it using js
yet using async in workers is still broken so let's use embind for now
*/
struct EmscriptenAsyncEval {
  static CBTypesInfo inputTypes() { return CoreInfo::StringType; }
  static CBTypesInfo outputTypes() { return CoreInfo::StringType; }

  CBVar activate(CBContext *context, const CBVar &input) {
    static thread_local std::string str;
    const static emscripten::val eval = emscripten::val::global("eval");
    str = emscripten_wait<std::string>(
        context, eval(emscripten::val(input.payload.stringValue)));
    return Var(str);
  }
};

// struct EmscriptenAsyncEval {
//   static CBTypesInfo inputTypes() { return CoreInfo::StringType; }
//   static CBTypesInfo outputTypes() { return CoreInfo::StringType; }

//   CBVar activate(CBContext *context, const CBVar &input) {
//     static thread_local std::string str;
//     static thread_local size_t slots;

//     size_t slot = ++slots;
//     const auto startCheck = emEvalAsyncRun(input.payload.stringValue, slot);
//     if (!startCheck) {
//       throw ActivationError("Failed to start a javascript async task.");
//     }

//     while (true) {
//       const auto runCheck = emEvalAsyncCheck(slot);
//       if (runCheck == 0) {
//         suspend(context, 0.0);
//       } else if (runCheck == -1) {
//         throw ActivationError("Failure on the javascript side, check
//         console");
//       } else {
//         break;
//       }
//     }

//     const auto res = emEvalAsyncGet(slot);
//     str.clear();
//     if (res) {
//       str.assign(res);
//       free(res);
//     }
//     return Var(str);
//   }
// };

CBVar emscriptenBrowseActivation(const CBVar &input) {
  emBrowsePage(input.payload.stringValue);
  return Var(input);
}
#endif

void registerBlocksCoreBlocks() {
  REGISTER_CBLOCK("Const", Const);
  REGISTER_CORE_BLOCK(Set);
  REGISTER_CORE_BLOCK(Ref);
  REGISTER_CORE_BLOCK(Update);
  REGISTER_CORE_BLOCK(Push);
  REGISTER_CORE_BLOCK(Sequence);
  chainblocks::registerBlock("Table", createBlockTableDecl);
  REGISTER_CORE_BLOCK(Clear);
  REGISTER_CORE_BLOCK(Pop);
  REGISTER_CORE_BLOCK(PopFront);
  REGISTER_CORE_BLOCK(Drop);
  REGISTER_CORE_BLOCK(DropFront);
  REGISTER_CORE_BLOCK(Count);
  REGISTER_CORE_BLOCK(Get);
  REGISTER_CORE_BLOCK(Swap);
  REGISTER_CORE_BLOCK(And);
  REGISTER_CORE_BLOCK(Or);
  REGISTER_CORE_BLOCK(Not);
  REGISTER_CORE_BLOCK(IsValidNumber);
  REGISTER_CORE_BLOCK(Take);
  REGISTER_CORE_BLOCK(RTake);
  REGISTER_CORE_BLOCK(Slice);
  REGISTER_CORE_BLOCK(Limit);
  REGISTER_CORE_BLOCK(Repeat);
  REGISTER_CORE_BLOCK(Sort);
  REGISTER_CORE_BLOCK(Remove);
  REGISTER_CORE_BLOCK(Is);
  REGISTER_CORE_BLOCK(IsNot);
  REGISTER_CORE_BLOCK(IsMore);
  REGISTER_CORE_BLOCK(IsLess);
  REGISTER_CORE_BLOCK(IsMoreEqual);
  REGISTER_CORE_BLOCK(IsLessEqual);
  REGISTER_CORE_BLOCK(Any);
  REGISTER_CORE_BLOCK(All);
  REGISTER_CORE_BLOCK(AnyNot);
  REGISTER_CORE_BLOCK(AllNot);
  REGISTER_CORE_BLOCK(AnyMore);
  REGISTER_CORE_BLOCK(AllMore);
  REGISTER_CORE_BLOCK(AnyLess);
  REGISTER_CORE_BLOCK(AllLess);
  REGISTER_CORE_BLOCK(AnyMoreEqual);
  REGISTER_CORE_BLOCK(AllMoreEqual);
  REGISTER_CORE_BLOCK(AnyLessEqual);
  REGISTER_CORE_BLOCK(AllLessEqual);

  REGISTER_CBLOCK("Profile", Profile);

  REGISTER_CBLOCK("ForEach", ForEachBlock);
  REGISTER_CBLOCK("Map", Map);
  REGISTER_CBLOCK("Reduce", Reduce);
  REGISTER_CBLOCK("Erase", Erase);
  REGISTER_CBLOCK("Once", Once);

  REGISTER_CBLOCK("Pause", Pause);
  REGISTER_CBLOCK("PauseMs", PauseMs);

  REGISTER_CBLOCK("AppendTo", AppendTo);
  REGISTER_CBLOCK("PrependTo", PrependTo);
  REGISTER_CBLOCK("Assoc", Assoc);

  using PassMockBlock =
      LambdaBlock<unreachableActivation, CoreInfo::AnyType, CoreInfo::AnyType>;
  using ExitBlock =
      LambdaBlock<exitProgramActivation, CoreInfo::IntType, CoreInfo::NoneType>;
  REGISTER_CBLOCK("Pass", PassMockBlock);
  REGISTER_CBLOCK("Exit", ExitBlock);

  using HasherBlock =
      LambdaBlock<hashActivation, CoreInfo::AnyType, CoreInfo::IntType>;
  REGISTER_CBLOCK("Hash", HasherBlock);

  using BlockingSleepBlock = LambdaBlock<blockingSleepActivation,
                                         CoreInfo::AnyType, CoreInfo::AnyType>;
  REGISTER_CBLOCK("SleepBlocking!", BlockingSleepBlock);

#ifdef __EMSCRIPTEN__
  using EmscriptenEvalBlock =
      LambdaBlock<emscriptenEvalActivation, CoreInfo::StringType,
                  CoreInfo::StringType>;
  // _ prefix = internal block
  REGISTER_CBLOCK("_Emscripten.Eval", EmscriptenEvalBlock);
  // _ prefix = internal block
  REGISTER_CBLOCK("_Emscripten.EvalAsync", EmscriptenAsyncEval);
  using EmscriptenBrowseBlock =
      LambdaBlock<emscriptenBrowseActivation, CoreInfo::StringType,
                  CoreInfo::StringType>;
  REGISTER_CBLOCK("Browse", EmscriptenBrowseBlock);
#endif

  REGISTER_CBLOCK("Return", Return);
  REGISTER_CBLOCK("Restart", Restart);
  REGISTER_CBLOCK("Fail", Fail);
  REGISTER_CBLOCK("NaNTo0", NaNTo0);
  REGISTER_CBLOCK("IsNone", IsNone);
  REGISTER_CBLOCK("IsNotNone", IsNotNone);
  REGISTER_CBLOCK("Input", Input);
  REGISTER_CBLOCK("Comment", Comment);
  REGISTER_CBLOCK("Replace", Replace);
  REGISTER_CBLOCK("OnCleanup", OnCleanup);
}
}; // namespace chainblocks
