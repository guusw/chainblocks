/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2019 Fragcolor Pte. Ltd. */

#ifndef CB_OPS_HPP
#define CB_OPS_HPP

#include <cassert>
#include <cfloat>
#include <chainblocks.hpp>
#include <string>
#include <string_view>

inline std::string type2Name(CBType type) {
  std::string name = "";
  switch (type) {
  case CBType::EndOfBlittableTypes:
    // this should never happen
    throw chainblocks::CBException("EndOfBlittableTypes is an invalid type");
  case CBType::None:
    name = "CBType::None";
    break;
  case CBType::Any:
    name = "CBType::Any";
    break;
  case CBType::Object:
    name = "CBType::Object";
    break;
  case CBType::Enum:
    name = "CBType::Enum";
    break;
  case CBType::Bool:
    name = "CBType::Bool";
    break;
  case CBType::Bytes:
    name = "CBType::Bytes";
    break;
  case CBType::Int:
    name = "CBType::Int";
    break;
  case CBType::Int2:
    name = "CBType::Int2";
    break;
  case CBType::Int3:
    name = "CBType::Int3";
    break;
  case CBType::Int4:
    name = "CBType::Int4";
    break;
  case CBType::Int8:
    name = "CBType::Int8";
    break;
  case CBType::Int16:
    name = "CBType::Int16";
    break;
  case CBType::Float:
    name = "CBType::Float";
    break;
  case CBType::Float2:
    name = "CBType::Float2";
    break;
  case CBType::Float3:
    name = "CBType::Float3";
    break;
  case CBType::Float4:
    name = "CBType::Float4";
    break;
  case CBType::Color:
    name = "CBType::Color";
    break;
  case CBType::Chain:
    name = "CBType::Chain";
    break;
  case CBType::Block:
    name = "CBType::Block";
    break;
  case CBType::String:
    name = "CBType::String";
    break;
  case CBType::ContextVar:
    name = "CBType::ContextVar";
    break;
  case CBType::Path:
    name = "CBType::Path";
    break;
  case CBType::Image:
    name = "CBType::Image";
    break;
  case CBType::Audio:
    name = "CBType::Audio";
    break;
  case CBType::Seq:
    name = "CBType::Seq";
    break;
  case CBType::Table:
    name = "CBType::Table";
    break;
  case CBType::Set:
    name = "CBType::Set";
    break;
  case CBType::Array:
    name = "CBType::Array";
    break;
  }
  return name;
}

ALWAYS_INLINE inline bool operator!=(const CBVar &a, const CBVar &b);
ALWAYS_INLINE inline bool operator<(const CBVar &a, const CBVar &b);
ALWAYS_INLINE inline bool operator>(const CBVar &a, const CBVar &b);
ALWAYS_INLINE inline bool operator>=(const CBVar &a, const CBVar &b);
ALWAYS_INLINE inline bool operator==(const CBVar &a, const CBVar &b);

bool operator==(const CBTypeInfo &a, const CBTypeInfo &b);
inline bool operator!=(const CBTypeInfo &a, const CBTypeInfo &b);

inline int cmp(const CBVar &a, const CBVar &b) {
  if (a == b)
    return 0;
  else if (a < b)
    return -1;
  else
    return 1;
}

bool _seqEq(const CBVar &a, const CBVar &b);

bool _setEq(const CBVar &a, const CBVar &b);

bool _tableEq(const CBVar &a, const CBVar &b);

ALWAYS_INLINE inline bool operator==(const CBVar &a, const CBVar &b) {
  if (a.valueType != b.valueType)
    return false;

  switch (a.valueType) {
  case CBType::None:
  case CBType::Any:
  case CBType::EndOfBlittableTypes:
    return true;
  case CBType::Object:
    return a.payload.objectVendorId == b.payload.objectVendorId &&
           a.payload.objectTypeId == b.payload.objectTypeId &&
           a.payload.objectValue == b.payload.objectValue;
  case CBType::Enum:
    return a.payload.enumVendorId == b.payload.enumVendorId &&
           a.payload.enumTypeId == b.payload.enumTypeId &&
           a.payload.enumValue == b.payload.enumValue;
  case CBType::Bool:
    return a.payload.boolValue == b.payload.boolValue;
  case CBType::Int:
    return a.payload.intValue == b.payload.intValue;
  case CBType::Float:
    return __builtin_fabs(a.payload.floatValue - b.payload.floatValue) <=
           FLT_EPSILON;
  case CBType::Int2: {
    CBInt2 vec = a.payload.int2Value == b.payload.int2Value;
    for (auto i = 0; i < 2; i++)
      if (vec[i] == 0)
        return false;
    return true;
  }
  case CBType::Int3: {
    CBInt3 vec = a.payload.int3Value == b.payload.int3Value;
    for (auto i = 0; i < 3; i++)
      if (vec[i] == 0)
        return false;
    return true;
  }
  case CBType::Int4: {
    CBInt4 vec = a.payload.int4Value == b.payload.int4Value;
    for (auto i = 0; i < 4; i++)
      if (vec[i] == 0)
        return false;
    return true;
  }
  case CBType::Int8: {
    CBInt8 vec = a.payload.int8Value == b.payload.int8Value;
    for (auto i = 0; i < 8; i++)
      if (vec[i] == 0)
        return false;
    return true;
  }
  case CBType::Int16: {
    CBInt16 vec = a.payload.int16Value == b.payload.int16Value;
    for (auto i = 0; i < 16; i++)
      if (vec[i] == 0)
        return false;
    return true;
  }
  case CBType::Float2: {
    const CBFloat2 vepsi = {FLT_EPSILON, FLT_EPSILON};
    CBFloat2 diff = a.payload.float2Value - b.payload.float2Value;
    diff[0] = __builtin_fabs(diff[0]);
    diff[1] = __builtin_fabs(diff[1]);
    CBInt2 vec = diff <= vepsi;
    for (auto i = 0; i < 2; i++)
      if (vec[i] == 0)
        return false;
    return true;
  }
  case CBType::Float3: {
    const CBFloat3 vepsi = {FLT_EPSILON, FLT_EPSILON, FLT_EPSILON};
    CBFloat3 diff = a.payload.float3Value - b.payload.float3Value;
    diff[0] = __builtin_fabs(diff[0]);
    diff[1] = __builtin_fabs(diff[1]);
    diff[2] = __builtin_fabs(diff[2]);
    CBInt3 vec = diff <= vepsi;
    for (auto i = 0; i < 3; i++)
      if (vec[i] == 0)
        return false;
    return true;
  }
  case CBType::Float4: {
    const CBFloat4 vepsi = {FLT_EPSILON, FLT_EPSILON, FLT_EPSILON, FLT_EPSILON};
    CBFloat4 diff = a.payload.float4Value - b.payload.float4Value;
    diff[0] = __builtin_fabs(diff[0]);
    diff[1] = __builtin_fabs(diff[1]);
    diff[2] = __builtin_fabs(diff[2]);
    diff[3] = __builtin_fabs(diff[3]);
    CBInt4 vec = diff <= vepsi;
    for (auto i = 0; i < 4; i++)
      if (vec[i] == 0)
        return false;
    return true;
  }
  case CBType::Color:
    return a.payload.colorValue.r == b.payload.colorValue.r &&
           a.payload.colorValue.g == b.payload.colorValue.g &&
           a.payload.colorValue.b == b.payload.colorValue.b &&
           a.payload.colorValue.a == b.payload.colorValue.a;
  case CBType::Chain:
    return a.payload.chainValue == b.payload.chainValue;
  case CBType::Block:
    return a.payload.blockValue == b.payload.blockValue;
  case CBType::Path:
  case CBType::ContextVar:
  case CBType::String: {
    if (a.payload.stringValue == b.payload.stringValue)
      return true;

    const auto astr =
        a.payload.stringLen > 0
            ? std::string_view(a.payload.stringValue, a.payload.stringLen)
            : std::string_view(a.payload.stringValue);

    const auto bstr =
        b.payload.stringLen > 0
            ? std::string_view(b.payload.stringValue, b.payload.stringLen)
            : std::string_view(b.payload.stringValue);

    return astr == bstr;
  }
  case CBType::Image: {
    auto apixsize = 1;
    auto bpixsize = 1;
    if ((a.payload.imageValue.flags & CBIMAGE_FLAGS_16BITS_INT) ==
        CBIMAGE_FLAGS_16BITS_INT)
      apixsize = 2;
    else if ((a.payload.imageValue.flags & CBIMAGE_FLAGS_32BITS_FLOAT) ==
             CBIMAGE_FLAGS_32BITS_FLOAT)
      apixsize = 4;
    if ((b.payload.imageValue.flags & CBIMAGE_FLAGS_16BITS_INT) ==
        CBIMAGE_FLAGS_16BITS_INT)
      bpixsize = 2;
    else if ((b.payload.imageValue.flags & CBIMAGE_FLAGS_32BITS_FLOAT) ==
             CBIMAGE_FLAGS_32BITS_FLOAT)
      bpixsize = 4;
    return apixsize == bpixsize &&
           a.payload.imageValue.channels == b.payload.imageValue.channels &&
           a.payload.imageValue.width == b.payload.imageValue.width &&
           a.payload.imageValue.height == b.payload.imageValue.height &&
           (a.payload.imageValue.data == b.payload.imageValue.data ||
            (memcmp(a.payload.imageValue.data, b.payload.imageValue.data,
                    a.payload.imageValue.channels * a.payload.imageValue.width *
                        a.payload.imageValue.height * apixsize) == 0));
  }
  case CBType::Audio: {
    return a.payload.audioValue.nsamples == b.payload.audioValue.nsamples &&
           a.payload.audioValue.channels == b.payload.audioValue.channels &&
           a.payload.audioValue.sampleRate == b.payload.audioValue.sampleRate &&
           (a.payload.audioValue.samples == b.payload.audioValue.samples ||
            (memcmp(a.payload.audioValue.samples, b.payload.audioValue.samples,
                    a.payload.audioValue.channels *
                        a.payload.audioValue.nsamples * sizeof(float)) == 0));
  }
  case CBType::Seq:
    return _seqEq(a, b);
  case CBType::Table:
    return _tableEq(a, b);
  case CBType::Set:
    return _setEq(a, b);
  case CBType::Bytes:
    return a.payload.bytesSize == b.payload.bytesSize &&
           (a.payload.bytesValue == b.payload.bytesValue ||
            memcmp(a.payload.bytesValue, b.payload.bytesValue,
                   a.payload.bytesSize) == 0);
  case CBType::Array:
    return a.payload.arrayValue.len == b.payload.arrayValue.len &&
           a.innerType == b.innerType &&
           (a.payload.arrayValue.elements == b.payload.arrayValue.elements ||
            memcmp(a.payload.arrayValue.elements, b.payload.arrayValue.elements,
                   a.payload.arrayValue.len * sizeof(CBVarPayload)) == 0);
  }

  return false;
}

bool _seqLess(const CBVar &a, const CBVar &b);

bool _tableLess(const CBVar &a, const CBVar &b);

// avoid trying to be smart with SIMDs here
// compiler will outsmart us likely anyway.
#define CBVECTOR_CMP(_a_, _b_, _s_, _final_)                                   \
  for (auto i = 0; i < _s_; i++) {                                             \
    if (_a_[i] < _b_[i])                                                       \
      return true;                                                             \
    else if (_a_[i] > _b_[i])                                                  \
      return false;                                                            \
  }                                                                            \
  return _final_

ALWAYS_INLINE inline bool operator<(const CBVar &a, const CBVar &b) {
  if (a.valueType != b.valueType)
    throw chainblocks::InvalidVarTypeError(
        "Comparison < between two different value types");

  switch (a.valueType) {
  case CBType::Enum: {
    if (a.payload.enumVendorId != b.payload.enumVendorId ||
        a.payload.enumTypeId != b.payload.enumTypeId)
      throw chainblocks::InvalidVarTypeError(
          "Comparison < between two different kind of enums (vendor/type)");
    return a.payload.enumValue < b.payload.enumValue;
  }
  case CBType::Bool:
    return a.payload.boolValue < b.payload.boolValue;
  case CBType::Int:
    return a.payload.intValue < b.payload.intValue;
  case CBType::Float:
    return a.payload.floatValue < b.payload.floatValue;
  case CBType::Int2: {
    CBVECTOR_CMP(a.payload.int2Value, b.payload.int2Value, 2, false);
  }
  case CBType::Int3: {
    CBVECTOR_CMP(a.payload.int3Value, b.payload.int3Value, 3, false);
  }
  case CBType::Int4: {
    CBVECTOR_CMP(a.payload.int4Value, b.payload.int4Value, 4, false);
  }
  case CBType::Int8: {
    CBVECTOR_CMP(a.payload.int8Value, b.payload.int8Value, 8, false);
  }
  case CBType::Int16: {
    CBVECTOR_CMP(a.payload.int16Value, b.payload.int16Value, 16, false);
  }
  case CBType::Float2: {
    CBVECTOR_CMP(a.payload.float2Value, b.payload.float2Value, 2, false);
  }
  case CBType::Float3: {
    CBVECTOR_CMP(a.payload.float3Value, b.payload.float3Value, 3, false);
  }
  case CBType::Float4: {
    CBVECTOR_CMP(a.payload.float4Value, b.payload.float4Value, 4, false);
  }
  case CBType::Color:
    return a.payload.colorValue.r < b.payload.colorValue.r ||
           a.payload.colorValue.g < b.payload.colorValue.g ||
           a.payload.colorValue.b < b.payload.colorValue.b ||
           a.payload.colorValue.a < b.payload.colorValue.a;
  case CBType::Path:
  case CBType::ContextVar:
  case CBType::String: {
    if (a.payload.stringValue == b.payload.stringValue)
      return false;

    const auto astr =
        a.payload.stringLen > 0
            ? std::string_view(a.payload.stringValue, a.payload.stringLen)
            : std::string_view(a.payload.stringValue);

    const auto bstr =
        b.payload.stringLen > 0
            ? std::string_view(b.payload.stringValue, b.payload.stringLen)
            : std::string_view(b.payload.stringValue);

    return astr < bstr;
  }
  case CBType::Seq:
    return _seqLess(a, b);
  case CBType::Table:
    return _tableLess(a, b);
  case CBType::Bytes: {
    if (a.payload.bytesValue == b.payload.bytesValue &&
        a.payload.bytesSize == b.payload.bytesSize)
      return false;
    std::string_view abuf((const char *)a.payload.bytesValue,
                          a.payload.bytesSize);
    std::string_view bbuf((const char *)b.payload.bytesValue,
                          b.payload.bytesSize);
    return abuf < bbuf;
  }
  case CBType::Array: {
    if (a.payload.arrayValue.elements == b.payload.arrayValue.elements &&
        a.payload.arrayValue.len == b.payload.arrayValue.len)
      return false;
    std::string_view abuf((const char *)a.payload.arrayValue.elements,
                          a.payload.arrayValue.len * sizeof(CBVarPayload));
    std::string_view bbuf((const char *)b.payload.arrayValue.elements,
                          b.payload.arrayValue.len * sizeof(CBVarPayload));
    return abuf < bbuf;
  }
  default:
    throw chainblocks::InvalidVarTypeError(
        "Comparison operator < not supported for the given type: " +
        type2Name(a.valueType));
  }
}

bool _seqLessEq(const CBVar &a, const CBVar &b);

bool _tableLessEq(const CBVar &a, const CBVar &b);

ALWAYS_INLINE inline bool operator<=(const CBVar &a, const CBVar &b) {
  if (a.valueType != b.valueType)
    throw chainblocks::InvalidVarTypeError(
        "Comparison <= between two different value types");

  switch (a.valueType) {
  case CBType::Enum: {
    if (a.payload.enumVendorId != b.payload.enumVendorId ||
        a.payload.enumTypeId != b.payload.enumTypeId)
      throw chainblocks::InvalidVarTypeError(
          "Comparison <= between two different kind of enums (vendor/type)");
    return a.payload.enumValue <= b.payload.enumValue;
  }
  case CBType::Bool:
    return a.payload.boolValue <= b.payload.boolValue;
  case CBType::Int:
    return a.payload.intValue <= b.payload.intValue;
  case CBType::Float:
    return a.payload.floatValue <= b.payload.floatValue;
  case CBType::Int2: {
    CBVECTOR_CMP(a.payload.int2Value, b.payload.int2Value, 2, true);
  }
  case CBType::Int3: {
    CBVECTOR_CMP(a.payload.int3Value, b.payload.int3Value, 3, true);
  }
  case CBType::Int4: {
    CBVECTOR_CMP(a.payload.int4Value, b.payload.int4Value, 4, true);
  }
  case CBType::Int8: {
    CBVECTOR_CMP(a.payload.int8Value, b.payload.int8Value, 8, true);
  }
  case CBType::Int16: {
    CBVECTOR_CMP(a.payload.int16Value, b.payload.int16Value, 16, true);
  }
  case CBType::Float2: {
    CBVECTOR_CMP(a.payload.float2Value, b.payload.float2Value, 2, true);
  }
  case CBType::Float3: {
    CBVECTOR_CMP(a.payload.float3Value, b.payload.float3Value, 3, true);
  }
  case CBType::Float4: {
    CBVECTOR_CMP(a.payload.float4Value, b.payload.float4Value, 4, true);
  }
  case CBType::Color:
    return a.payload.colorValue.r <= b.payload.colorValue.r &&
           a.payload.colorValue.g <= b.payload.colorValue.g &&
           a.payload.colorValue.b <= b.payload.colorValue.b &&
           a.payload.colorValue.a <= b.payload.colorValue.a;
  case CBType::Path:
  case CBType::ContextVar:
  case CBType::String: {
    if (a.payload.stringValue == b.payload.stringValue)
      return true;

    const auto astr =
        a.payload.stringLen > 0
            ? std::string_view(a.payload.stringValue, a.payload.stringLen)
            : std::string_view(a.payload.stringValue);

    const auto bstr =
        b.payload.stringLen > 0
            ? std::string_view(b.payload.stringValue, b.payload.stringLen)
            : std::string_view(b.payload.stringValue);

    return astr <= bstr;
  }
  case CBType::Seq:
    return _seqLessEq(a, b);
  case CBType::Table:
    return _tableLessEq(a, b);
  case CBType::Bytes: {
    if (a.payload.bytesValue == b.payload.bytesValue &&
        a.payload.bytesSize == b.payload.bytesSize)
      return true;
    std::string_view abuf((const char *)a.payload.bytesValue,
                          a.payload.bytesSize);
    std::string_view bbuf((const char *)b.payload.bytesValue,
                          b.payload.bytesSize);
    return abuf <= bbuf;
  }
  case CBType::Array: {
    if (a.payload.arrayValue.elements == b.payload.arrayValue.elements &&
        a.payload.arrayValue.len == b.payload.arrayValue.len)
      return true;
    std::string_view abuf((const char *)a.payload.arrayValue.elements,
                          a.payload.arrayValue.len * sizeof(CBVarPayload));
    std::string_view bbuf((const char *)b.payload.arrayValue.elements,
                          b.payload.arrayValue.len * sizeof(CBVarPayload));
    return abuf <= bbuf;
  }
  default:
    throw chainblocks::InvalidVarTypeError(
        "Comparison operator <= not supported for the given type: " +
        type2Name(a.valueType));
  }
}

#undef CBVECTOR_CMP

ALWAYS_INLINE inline bool operator!=(const CBVar &a, const CBVar &b) {
  return !(a == b);
}

ALWAYS_INLINE inline bool operator>(const CBVar &a, const CBVar &b) {
  return b < a;
}

ALWAYS_INLINE inline bool operator>=(const CBVar &a, const CBVar &b) {
  return b <= a;
}

inline bool operator!=(const CBTypeInfo &a, const CBTypeInfo &b) {
  return !(a == b);
}

inline bool operator!=(const CBExposedTypeInfo &a, const CBExposedTypeInfo &b);

inline bool operator==(const CBExposedTypeInfo &a, const CBExposedTypeInfo &b) {
  if (strcmp(a.name, b.name) != 0 || a.exposedType != b.exposedType ||
      a.isMutable != b.isMutable || a.isProtected != b.isProtected ||
      a.global != b.global)
    return false;
  return true;
}

inline bool operator!=(const CBExposedTypeInfo &a, const CBExposedTypeInfo &b) {
  return !(a == b);
}

namespace chainblocks {
uint64_t hash(const CBVar &var);
} // namespace chainblocks

namespace std {
template <> struct hash<CBVar> {
  std::size_t operator()(const CBVar &var) const {
    // not ideal on 32 bits as our hash is 64.. but it should be ok
    return std::size_t(chainblocks::hash(var));
  }
};

template <> struct hash<CBTypeInfo> {
  std::size_t operator()(const CBTypeInfo &typeInfo) const {
    using std::hash;
    using std::size_t;
    using std::string;
    auto res = hash<int>()((int)typeInfo.basicType);
    res = res ^ hash<int>()((int)typeInfo.innerType);
    if (typeInfo.basicType == CBType::Table) {
      if (typeInfo.table.keys.elements) {
        for (uint32_t i = 0; i < typeInfo.table.keys.len; i++) {
          res = res ^ hash<string>()(typeInfo.table.keys.elements[i]);
        }
      }
      if (typeInfo.table.types.elements) {
        for (uint32_t i = 0; i < typeInfo.table.types.len; i++) {
          res = res ^ hash<CBTypeInfo>()(typeInfo.table.types.elements[i]);
        }
      }
    } else if (typeInfo.basicType == CBType::Seq) {
      for (uint32_t i = 0; i < typeInfo.seqTypes.len; i++) {
        if (typeInfo.seqTypes.elements[i].recursiveSelf) {
          res = res ^ hash<int>()(INT32_MAX);
        } else {
          res = res ^ hash<CBTypeInfo>()(typeInfo.seqTypes.elements[i]);
        }
      }
    } else if (typeInfo.basicType == CBType::Set) {
      for (uint32_t i = 0; i < typeInfo.setTypes.len; i++) {
        if (typeInfo.setTypes.elements[i].recursiveSelf) {
          res = res ^ hash<int>()(INT32_MAX);
        } else {
          res = res ^ hash<CBTypeInfo>()(typeInfo.setTypes.elements[i]);
        }
      }
    } else if (typeInfo.basicType == CBType::Object) {
      res = res ^ hash<int>()(typeInfo.object.vendorId);
      res = res ^ hash<int>()(typeInfo.object.typeId);
    } else if (typeInfo.basicType == CBType::Enum) {
      res = res ^ hash<int>()(typeInfo.enumeration.vendorId);
      res = res ^ hash<int>()(typeInfo.enumeration.typeId);
    }
    return res;
  }
};

template <> struct hash<CBExposedTypeInfo> {
  std::size_t operator()(const CBExposedTypeInfo &typeInfo) const {
    using std::hash;
    using std::size_t;
    using std::string;
    auto res = hash<string>()(typeInfo.name);
    res = res ^ hash<CBTypeInfo>()(typeInfo.exposedType);
    res = res ^ hash<int>()(typeInfo.isMutable);
    res = res ^ hash<int>()(typeInfo.isProtected);
    res = res ^ hash<int>()(typeInfo.isTableEntry);
    res = res ^ hash<int>()(typeInfo.global);
    return res;
  }
};
} // namespace std

#endif
