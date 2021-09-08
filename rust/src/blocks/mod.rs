/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2020 Fragcolor Pte. Ltd. */

#[cfg(not(target_arch = "wasm32"))]
extern crate reqwest;

extern crate bs58;
extern crate hex;
extern crate resvg;
extern crate libsecp256k1;
extern crate sha2;
extern crate tiny_keccak;
extern crate tiny_skia;
extern crate usvg;
extern crate wasabi_leb128;
extern crate ethabi;
extern crate serde_json;
extern crate ethereum_types;

#[cfg(not(any(target_arch = "wasm32", target_os = "ios")))]
extern crate webbrowser;

use crate::types::common_type;
use crate::types::Type;

#[cfg(not(target_arch = "wasm32"))]
pub mod http;

pub mod hash;
pub mod ecdsa;

pub mod physics;

#[cfg(not(any(target_arch = "wasm32", target_os = "ios")))]
pub mod browse;

pub mod casting;

pub mod svg;

pub mod eth;