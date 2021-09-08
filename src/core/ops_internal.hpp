/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2019 Fragcolor Pte. Ltd. */

#ifndef CB_OPS_INTERNAL_HPP
#define CB_OPS_INTERNAL_HPP

#include <ops.hpp>

#include <spdlog/spdlog.h>

#include "spdlog/fmt/ostr.h" // must be included

CHAINBLOCKS_API std::ostream &operator<<(std::ostream &os, const CBVar &var);
CHAINBLOCKS_API std::ostream &operator<<(std::ostream &os, const CBTypeInfo &t);
CHAINBLOCKS_API std::ostream &operator<<(std::ostream &os, const CBTypesInfo &ts);

#endif
