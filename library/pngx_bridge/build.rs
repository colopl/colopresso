/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of colopresso
 *
 * Copyright (C) 2025 COLOPL, Inc.
 *
 * Author: Go Kudo <g-kudo@colopl.co.jp>
 * Developed with AI (LLM) code assistance. See `NOTICE` for details.
 */

use std::env;
use std::fs;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=cbindgen.toml");

    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let crate_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());

    let header_path = out_dir.join("pngx_bridge.h");

    let export_dir = crate_dir.join("generated");
    if let Err(e) = fs::create_dir_all(&export_dir) {
        panic!("Failed to create export dir {export_dir:?}: {e}");
    }
    let export_header_path = export_dir.join("pngx_bridge.h");

    match cbindgen::generate(crate_dir) {
        Ok(bindings) => {
            bindings.write_to_file(&header_path);
            if let Err(e) = fs::copy(&header_path, &export_header_path) {
                panic!("Failed to copy header to stable path {export_header_path:?}: {e}");
            }
            println!(
                "cargo:warning=Generated header -> {} (stable copy {})",
                header_path.display(),
                export_header_path.display()
            );
        }
        Err(e) => {
            panic!("Failed to generate bindings: {e}");
        }
    }
}
