/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2019 Fragcolor Pte. Ltd. */

// #include "shared.hpp"
// #include <boost/process/environment.hpp>

// namespace chainblocks {
// namespace OS {
// struct GetEnv {
//   std::string _value;

//   static CBTypesInfo inputTypes() { return CoreInfo::NoneType; }
//   static CBTypesInfo outputTypes() { return CoreInfo::StringType; }

//    CBVar activate(CBContext *context, const CBVar &input) {
//     auto envs = boost::this_process::environment();
//     _value = envs[_name(context).payload.stringValue].to_string();
//     return Var(_value);
//   }
// };

// struct SetEnv {
//   static CBTypesInfo inputTypes() { return CoreInfo::StringType; }
//   static CBTypesInfo outputTypes() { return CoreInfo::StringType; }

//    CBVar activate(CBContext *context, const CBVar &input) {
//     auto envs = boost::this_process::environment();
//     envs[_name(context).payload.stringValue] = input.payload.stringValue;
//     return input;
//   }
// };

// struct AddEnv {
//   static CBTypesInfo inputTypes() { return CoreInfo::StringType; }
//   static CBTypesInfo outputTypes() { return CoreInfo::StringType; }

//    CBVar activate(CBContext *context, const CBVar &input) {
//     auto envs = boost::this_process::environment();
//     envs[_name(context).payload.stringValue] += input.payload.stringValue;
//     return input;
//   }
// };

// struct UnsetEnv {
//   static CBTypesInfo inputTypes() { return CoreInfo::AnyType; }
//   static CBTypesInfo outputTypes() { return CoreInfo::AnyType; }

//    CBVar activate(CBContext *context, const CBVar &input) {
//     auto envs = boost::this_process::environment();
//     envs[_name(context).payload.stringValue].clear();
//     return input;
//   }
// };

// } // namespace OS

// void registerOSBlocks() {
//   REGISTER_CBLOCK("OS.GetEnv", OS::GetEnv);
//   REGISTER_CBLOCK("OS.SetEnv", OS::SetEnv);
//   REGISTER_CBLOCK("OS.AddEnv", OS::AddEnv);
//   REGISTER_CBLOCK("OS.UnsetEnv", OS::UnsetEnv);
// }
// } // namespace chainblocks
