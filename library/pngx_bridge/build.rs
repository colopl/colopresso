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

fn parse_version_to_u32(version: &str) -> u32 {
    let parts: Vec<&str> = version.split('.').collect();
    let major: u32 = parts.first().and_then(|s| s.parse().ok()).unwrap_or(0);
    let minor: u32 = parts.get(1).and_then(|s| s.parse().ok()).unwrap_or(0);
    let patch: u32 = parts.get(2).and_then(|s| s.parse().ok()).unwrap_or(0);
    major * 10000 + minor * 100 + patch
}

fn extract_version_from_lock(lock_content: &str, package_name: &str) -> Option<String> {
    let mut in_target_package = false;
    for line in lock_content.lines() {
        let trimmed = line.trim();
        if trimmed.starts_with("[[package]]") {
            in_target_package = false;
        } else if trimmed.starts_with("name = ") {
            let name = trimmed.trim_start_matches("name = ").trim_matches('"');
            if name == package_name {
                in_target_package = true;
            }
        } else if in_target_package && trimmed.starts_with("version = ") {
            let version = trimmed.trim_start_matches("version = ").trim_matches('"');
            return Some(version.to_string());
        }
    }
    None
}

fn main() {
    println!("cargo:rustc-check-cfg=cfg(pngx_bridge_msan)");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=cbindgen.toml");
    println!("cargo:rerun-if-changed=Cargo.lock");

    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let crate_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());

    let lock_path = crate_dir.join("Cargo.lock");
    if let Ok(lock_content) = fs::read_to_string(&lock_path) {
        if let Some(ver) = extract_version_from_lock(&lock_content, "oxipng") {
            let ver_u32 = parse_version_to_u32(&ver);
            println!("cargo:rustc-env=PNGX_BRIDGE_OXIPNG_VERSION={}", ver_u32);
        }
        if let Some(ver) = extract_version_from_lock(&lock_content, "imagequant") {
            let ver_u32 = parse_version_to_u32(&ver);
            println!("cargo:rustc-env=PNGX_BRIDGE_IMAGEQUANT_VERSION={}", ver_u32);
        }
    }

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
