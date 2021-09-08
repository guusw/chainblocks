/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2019 Fragcolor Pte. Ltd. */

#include "shared.hpp"
#include <boost/algorithm/string.hpp>
#include <fstream>

#ifdef WIN32
// windows mingw has bugged copy/copyfile
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

namespace chainblocks {
namespace FS {
struct Iterate {
  CBSeq _storage = {};
  std::vector<std::string> _strings;

  void destroy() {
    if (_storage.elements) {
      chainblocks::arrayFree(_storage);
    }
  }

  static CBTypesInfo inputTypes() { return CoreInfo::StringType; }
  static CBTypesInfo outputTypes() { return CoreInfo::StringSeqType; }

  bool _recursive = true;

  static inline ParamsInfo params = ParamsInfo(ParamsInfo::Param(
      "Recursive",
      CBCCSTR(
          "If the iteration should be recursive, following sub-directories."),
      CoreInfo::BoolType));
  static CBParametersInfo parameters() { return CBParametersInfo(params); }

  void setParam(int index, const CBVar &value) {
    switch (index) {
    case 0:
      _recursive = bool(Var(value));
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
      return Var(_recursive);
    default:
      return Var::Empty;
    }
  }

  CBVar activate(CBContext *context, const CBVar &input) {
    chainblocks::arrayResize(_storage, 0);
    _strings.clear();

    if (_recursive) {
      fs::path p(input.payload.stringValue);
      auto diterator = fs::recursive_directory_iterator(p);
      for (auto &subp : diterator) {
        auto &path = subp.path();
        auto str = path.string();
#ifdef _WIN32
        boost::replace_all(str, "\\", "/");
#endif
        _strings.push_back(str);
      }
    } else {
      fs::path p(input.payload.stringValue);
      auto diterator = fs::directory_iterator(p);
      for (auto &subp : diterator) {
        auto &path = subp.path();
        auto str = path.string();
#ifdef _WIN32
        boost::replace_all(str, "\\", "/");
#endif
        _strings.push_back(str);
      }
    }

    chainblocks::arrayResize(_storage, 0);
    for (auto &sref : _strings) {
      chainblocks::arrayPush(_storage, Var(sref.c_str()));
    }

    return Var(_storage);
  }
};

struct Extension {
  std::string _output;

  static CBTypesInfo inputTypes() { return CoreInfo::StringType; }
  static CBTypesInfo outputTypes() { return CoreInfo::StringType; }

  CBVar activate(CBContext *context, const CBVar &input) {
    _output.clear();
    fs::path p(input.payload.stringValue);
    auto ext = p.extension();
    _output.assign(ext.string());
    return Var(_output);
  }
};

struct IsFile {
  static CBTypesInfo inputTypes() { return CoreInfo::StringType; }
  static CBTypesInfo outputTypes() { return CoreInfo::BoolType; }
  CBVar activate(CBContext *context, const CBVar &input) {
    fs::path p(input.payload.stringValue);
    if (fs::exists(p) && !fs::is_directory(p)) {
      return Var::True;
    } else {
      return Var::False;
    }
  }
};

struct IsDirectory {
  static CBTypesInfo inputTypes() { return CoreInfo::StringType; }
  static CBTypesInfo outputTypes() { return CoreInfo::BoolType; }
  CBVar activate(CBContext *context, const CBVar &input) {
    fs::path p(input.payload.stringValue);
    if (fs::exists(p) && fs::is_directory(p)) {
      return Var::True;
    } else {
      return Var::False;
    }
  }
};

struct Remove {
  static CBTypesInfo inputTypes() { return CoreInfo::StringType; }
  static CBTypesInfo outputTypes() { return CoreInfo::BoolType; }
  CBVar activate(CBContext *context, const CBVar &input) {
    fs::path p(input.payload.stringValue);
    if (fs::exists(p)) {
      return Var(fs::remove(p));
    } else {
      return Var::False;
    }
  }
};

struct Filename {
  std::string _output;
  bool _noExt = false;

  static CBTypesInfo inputTypes() { return CoreInfo::StringType; }
  static CBTypesInfo outputTypes() { return CoreInfo::StringType; }

  static inline ParamsInfo params = ParamsInfo(ParamsInfo::Param(
      "NoExtension",
      CBCCSTR("If the extension should be stripped from the result."),
      CoreInfo::BoolType));
  static CBParametersInfo parameters() { return CBParametersInfo(params); }

  void setParam(int index, const CBVar &value) {
    switch (index) {
    case 0:
      _noExt = bool(Var(value));
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
      return Var(_noExt);
    default:
      return Var::Empty;
    }
  }

  CBVar activate(CBContext *context, const CBVar &input) {
    _output.clear();
    fs::path p(input.payload.stringValue);
    p = p.filename();
    if (_noExt && p.has_extension()) {
      p = p.replace_extension("");
    }
    _output.assign(p.string());
    return Var(_output);
  }
};

struct Read {
  std::vector<uint8_t> _buffer;
  bool _binary = false;

  static CBTypesInfo inputTypes() { return CoreInfo::StringType; }
  CBTypesInfo outputTypes() {
    if (_binary)
      return CoreInfo::BytesType;
    else
      return CoreInfo::StringType;
  }

  static inline ParamsInfo params = ParamsInfo(ParamsInfo::Param(
      "Bytes", CBCCSTR("If the output should be Bytes instead of String."),
      CoreInfo::BoolType));
  static CBParametersInfo parameters() { return CBParametersInfo(params); }

  void setParam(int index, const CBVar &value) {
    switch (index) {
    case 0:
      _binary = bool(Var(value));
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
      return Var(_binary);
    default:
      return Var::Empty;
    }
  }

  CBVar activate(CBContext *context, const CBVar &input) {
    _buffer.clear();
    fs::path p(input.payload.stringValue);
    if (!fs::exists(p)) {
      CBLOG_ERROR("File is missing: {}", p);
      throw ActivationError("FS.Read, file does not exist.");
    }

    if (_binary) {
      std::ifstream file(p.string(), std::ios::binary);
      _buffer.assign(std::istreambuf_iterator<char>(file), {});
      return Var(_buffer.data(), uint32_t(_buffer.size()));
    } else {
      std::ifstream file(p.string(), std::ios::binary);
      _buffer.assign(std::istreambuf_iterator<char>(file), {});
      _buffer.push_back(0);
      return Var((const char *)_buffer.data());
    }
  }
};

struct Write {
  ParamVar _contents{};
  std::array<CBExposedTypeInfo, 2> _requiring;
  bool _overwrite = false;
  bool _append = false;

  static CBTypesInfo inputTypes() { return CoreInfo::StringType; }
  static CBTypesInfo outputTypes() { return CoreInfo::StringType; }

  static inline Parameters params{
      {"Contents",
       CBCCSTR("The string or bytes to write as the file's contents."),
       {CoreInfo::StringType, CoreInfo::BytesType, CoreInfo::StringVarType,
        CoreInfo::BytesVarType, CoreInfo::NoneType}},
      {"Overwrite",
       CBCCSTR("Overwrite the file if it already exists."),
       {CoreInfo::BoolType}},
      {"Append",
       CBCCSTR("If we should append Contents to an existing file."),
       {CoreInfo::BoolType}}};

  static CBParametersInfo parameters() { return params; }

  void setParam(int index, const CBVar &value) {
    switch (index) {
    case 0:
      _contents = value;
      break;
    case 1:
      _overwrite = value.payload.boolValue;
      break;
    case 2:
      _append = value.payload.boolValue;
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
      return _contents;
    case 1:
      return Var(_overwrite);
    case 2:
      return Var(_append);
    default:
      return Var::Empty;
    }
  }

  CBExposedTypesInfo requiredVariables() {
    if (_contents.isVariable()) {
      _requiring[0].name = _contents.variableName();
      _requiring[0].help =
          CBCCSTR("The required variable containing the data to be written.");
      _requiring[0].exposedType = CoreInfo::StringType;
      _requiring[1].name = _contents.variableName();
      _requiring[1].help =
          CBCCSTR("The required variable containing the data to be written.");
      _requiring[1].exposedType = CoreInfo::BytesType;
      return {_requiring.data(), 2, 0};
    } else {
      return {};
    }
  }

  void cleanup() { _contents.cleanup(); }
  void warmup(CBContext *context) { _contents.warmup(context); }

  CBVar activate(CBContext *context, const CBVar &input) {
    auto contents = _contents.get();
    if (contents.valueType != None) {
      fs::path p(input.payload.stringValue);
      if (!_overwrite && !_append && fs::exists(p)) {
        throw ActivationError(
            "FS.Write, file already exists and overwrite flag is not on!.");
      }

      // make sure to create directories
      auto parent_path = p.parent_path();
      if (!parent_path.empty() && !fs::exists(parent_path))
        fs::create_directories(p.parent_path());

      auto flags = std::ios::binary;
      if (_append) {
        flags |= std::ios::app;
      }
      std::ofstream file(p.string(), flags);
      if (contents.valueType == String) {
        auto len = contents.payload.stringLen > 0 ||
                           contents.payload.stringValue == nullptr
                       ? contents.payload.stringLen
                       : strlen(contents.payload.stringValue);
        file.write((const char *)contents.payload.stringValue, len);
      } else {
        file.write((const char *)contents.payload.bytesValue,
                   contents.payload.bytesSize);
      }
    }
    return input;
  }
};

struct Copy {
  enum class OverBehavior { Fail, Skip, Overwrite, Update };
  static inline EnumInfo<OverBehavior> OverWEnum{"IfExists", CoreCC, 'fsow'};
  static inline Type OverWEnumType{
      {CBType::Enum, {.enumeration = {CoreCC, 'fsow'}}}};

  ParamVar _destination{};
  OverBehavior _overwrite{OverBehavior::Fail};

  static CBTypesInfo inputTypes() { return CoreInfo::StringType; }
  static CBTypesInfo outputTypes() { return CoreInfo::StringType; }

  static inline ParamsInfo params = ParamsInfo(
      ParamsInfo::Param(
          "Destination",
          CBCCSTR("The destination path, can be a file or a directory."),
          CoreInfo::StringStringVarOrNone),
      ParamsInfo::Param(
          "Behavior",
          CBCCSTR("What to do when the destination already exists."),
          OverWEnumType));
  static CBParametersInfo parameters() { return CBParametersInfo(params); }

  void setParam(int index, const CBVar &value) {
    switch (index) {
    case 0:
      _destination = value;
      break;
    case 1:
      _overwrite = OverBehavior(value.payload.enumValue);
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
      return _destination;
    case 1:
      return Var::Enum(_overwrite, CoreCC, 'fsow');
    default:
      return Var::Empty;
    }
  }

  void cleanup() { _destination.cleanup(); }
  void warmup(CBContext *context) { _destination.warmup(context); }

  CBVar activate(CBContext *context, const CBVar &input) {
    const auto src = fs::path(input.payload.stringValue);
    if (!fs::exists(src))
      throw ActivationError("Source path does not exist.");

    fs::copy_options options{};

    switch (_overwrite) {
    case OverBehavior::Fail:
      break;
    case OverBehavior::Skip:
      options |= fs::copy_options::skip_existing;
      break;
    case OverBehavior::Overwrite:
      options |= fs::copy_options::overwrite_existing;
      break;
    case OverBehavior::Update:
      options |= fs::copy_options::update_existing;
      break;
    }

    const auto dstVar = _destination.get();
    if (dstVar.valueType != String && dstVar.valueType != Path)
      throw ActivationError("Destination is not a valid");
    const auto dst = fs::path(dstVar.payload.stringValue);

    std::error_code err;
    if (fs::is_regular_file(src) &&
        (!fs::exists(dst) || fs::is_regular_file(dst))) {
      fs::copy_file(src, dst, options, err);
      if (err) {
        CBLOG_ERROR("copy error: {}", err.message());
        throw ActivationError("Copy failed.");
      }
    } else {
      options |= fs::copy_options::recursive;
      fs::copy(src, dst, options, err);
      if (err) {
        CBLOG_ERROR("copy error: {}", err.message());
        throw ActivationError("Copy failed.");
      }
    }

    return input;
  }
};
}; // namespace FS

void registerFSBlocks() {
  REGISTER_CBLOCK("FS.Iterate", FS::Iterate);
  REGISTER_CBLOCK("FS.Extension", FS::Extension);
  REGISTER_CBLOCK("FS.Filename", FS::Filename);
  REGISTER_CBLOCK("FS.Read", FS::Read);
  REGISTER_CBLOCK("FS.Write", FS::Write);
  REGISTER_CBLOCK("FS.IsFile", FS::IsFile);
  REGISTER_CBLOCK("FS.IsDirectory", FS::IsDirectory);
  REGISTER_CBLOCK("FS.Copy", FS::Copy);
  REGISTER_CBLOCK("FS.Remove", FS::Remove);
}
}; // namespace chainblocks
