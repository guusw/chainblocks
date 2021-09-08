/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2019 Fragcolor Pte. Ltd. */

#include "blocks/shared.hpp"
#include "runtime.hpp"
#include <snappy.h>

namespace chainblocks {
namespace Snappy {
struct Compress {
  std::vector<char> _buffer;

  static CBTypesInfo inputTypes() { return CoreInfo::BytesType; }
  static CBTypesInfo outputTypes() { return CoreInfo::BytesType; }

  CBVar activate(CBContext *context, const CBVar &input) {
    _buffer.resize(snappy::MaxCompressedLength(input.payload.bytesSize));
    size_t outputLen;
    snappy::RawCompress((char *)input.payload.bytesValue,
                        input.payload.bytesSize, &_buffer[0], &outputLen);
    return Var((uint8_t *)&_buffer[0], uint32_t(outputLen));
  }
};

struct Decompress {
  std::vector<char> _buffer;

  static CBTypesInfo inputTypes() { return CoreInfo::BytesType; }
  static CBTypesInfo outputTypes() { return CoreInfo::BytesType; }

  CBVar activate(CBContext *context, const CBVar &input) {
    size_t len;
    if (!snappy::GetUncompressedLength((char *)input.payload.bytesValue,
                                       size_t(input.payload.bytesSize), &len)) {
      throw CBException(
          "Snappy failed to find uncompressed length, probably invalid data!");
    }
    _buffer.resize(len + 1);
    snappy::RawUncompress((char *)input.payload.bytesValue,
                          input.payload.bytesSize, &_buffer[0]);
    // easy fix for null term strings
    _buffer[len] = 0;
    return Var((uint8_t *)&_buffer[0], uint32_t(len));
  }
};

void registerBlocks() {
  REGISTER_CBLOCK("Snappy.Compress", Compress);
  REGISTER_CBLOCK("Snappy.Decompress", Decompress);
}
} // namespace Snappy
} // namespace chainblocks