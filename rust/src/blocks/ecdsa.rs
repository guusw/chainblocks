/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2021 Fragcolor Pte. Ltd. */

use crate::block::Block;
use crate::chainblocksc::CBType_String;
use crate::core::do_blocking;
use crate::core::log;
use crate::core::registerBlock;
use crate::types::common_type;
use crate::types::ClonedVar;
use crate::types::Context;
use crate::types::InstanceData;
use crate::types::ParamVar;
use crate::types::Parameters;
use crate::types::Seq;
use crate::types::Table;
use crate::types::Type;
use crate::CString;
use crate::Types;
use crate::Var;
use core::time::Duration;
use std::convert::TryFrom;
use std::convert::TryInto;
use std::ffi::CStr;

lazy_static! {
  static ref INPUT_TYPES: Vec<Type> = vec![common_type::bytes];
  static ref OUTPUT_TYPE: Vec<Type> = vec![common_type::bytezs];
  static ref PARAMETERS: Parameters = vec![(
    cstr!("Key"),
    cbccstr!("The private key to be used to sign the hashed message input."),
    vec![
      common_type::bytes,
      common_type::bytes_var,
      common_type::string,
      common_type::string_var
    ]
  )
    .into()];
  static ref PK_TYPES: Vec<Type> = vec![common_type::bytes, common_type::string];
  static ref PK_PARAMETERS: Parameters = vec![(
    cstr!("Compressed"),
    cbccstr!("If the output PublicKey should use the compressed format."),
    vec![common_type::bool]
  )
    .into()];
}

fn get_key(input: Var) -> Result<libsecp256k1::SecretKey, &'static str> {
  let key: Result<&[u8], &str> = input.as_ref().try_into();
  if let Ok(key) = key {
    libsecp256k1::SecretKey::parse_slice(key).map_err(|e| {
      cblog!("{}", e);
      "Failed to parse secret key"
    })
  } else {
    let key: Result<&str, &str> = input.as_ref().try_into();
    if let Ok(key) = key {
      let key = hex::decode(key).map_err(|e| {
        cblog!("{}", e);
        "Failed to parse key's hex string"
      })?;
      libsecp256k1::SecretKey::parse_slice(key.as_slice()).map_err(|e| {
        cblog!("{}", e);
        "Failed to parse secret key"
      })
    } else {
      Err("Invalid key value")
    }
  }
}

#[derive(Default)]
struct ECDSASign {
  output: Seq,
  key: ParamVar,
}

impl Block for ECDSASign {
  fn registerName() -> &'static str {
    cstr!("ECDSA.Sign")
  }

  fn hash() -> u32 {
    compile_time_crc32::crc32!("ECDSA.Sign-rust-0x20200101")
  }

  fn name(&mut self) -> &str {
    "ECDSA.Sign"
  }

  fn inputTypes(&mut self) -> &std::vec::Vec<Type> {
    &INPUT_TYPES
  }

  fn outputTypes(&mut self) -> &std::vec::Vec<Type> {
    &OUTPUT_TYPE
  }

  fn parameters(&mut self) -> Option<&Parameters> {
    Some(&PARAMETERS)
  }

  fn setParam(&mut self, index: i32, value: &Var) {
    match index {
      0 => self.key.set_param(value),
      _ => unreachable!(),
    }
  }

  fn getParam(&mut self, index: i32) -> Var {
    match index {
      0 => self.key.get_param(),
      _ => unreachable!(),
    }
  }

  fn warmup(&mut self, context: &Context) -> Result<(), &str> {
    self.key.warmup(context);
    self.output.set_len(2);
    Ok(())
  }

  fn cleanup(&mut self) {
    self.key.cleanup();
  }

  fn activate(&mut self, _: &Context, input: &Var) -> Result<Var, &str> {
    let bytes: &[u8] = input.as_ref().try_into()?;

    let key = self.key.get();
    let key = get_key(key)?;

    let msg = libsecp256k1::Message::parse_slice(bytes).map_err(|e| {
      cblog!("{}", e);
      "Failed to parse input message hash"
    })?;

    let signed = libsecp256k1::sign(&msg, &key);
    let signature = signed.0.serialize();
    self.output[0] = signature[..].into();
    self.output[1] = signed.1.serialize().try_into()?;
    Ok(self.output.as_ref().into())
  }
}

#[derive(Default)]
struct ECDSAPubKey {
  output: ClonedVar,
  is_string: bool,
  compressed: bool,
}

impl Block for ECDSAPubKey {
  fn registerName() -> &'static str {
    cstr!("ECDSA.PublicKey")
  }

  fn hash() -> u32 {
    compile_time_crc32::crc32!("ECDSA.PublicKey-rust-0x20200101")
  }

  fn name(&mut self) -> &str {
    "ECDSA.PublicKey"
  }

  fn inputTypes(&mut self) -> &std::vec::Vec<Type> {
    &PK_TYPES
  }

  fn outputTypes(&mut self) -> &std::vec::Vec<Type> {
    &INPUT_TYPES
  }

  fn parameters(&mut self) -> Option<&Parameters> {
    Some(&PK_PARAMETERS)
  }

  fn setParam(&mut self, index: i32, value: &Var) {
    match index {
      0 => self.compressed = value.try_into().unwrap(),
      _ => unreachable!(),
    }
  }

  fn getParam(&mut self, index: i32) -> Var {
    match index {
      0 => self.compressed.into(),
      _ => unreachable!(),
    }
  }

  fn activate(&mut self, _: &Context, input: &Var) -> Result<Var, &str> {
    let key = get_key(*input)?;
    let key = libsecp256k1::PublicKey::from_secret_key(&key);
    if !self.compressed {
      let key: [u8; 65] = key.serialize();
      self.output = key[..].into();
    } else {
      let key: [u8; 33] = key.serialize_compressed();
      self.output = key[..].into();
    }
    Ok(self.output.0)
  }
}

pub fn registerBlocks() {
  registerBlock::<ECDSASign>();
  registerBlock::<ECDSAPubKey>();
}
