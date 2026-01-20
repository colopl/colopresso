/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of colopresso
 *
 * Copyright (C) 2025-2026 COLOPL, Inc.
 *
 * Author: Go Kudo <g-kudo@colopl.co.jp>
 * Developed with AI (LLM) code assistance. See `NOTICE` for details.
 */

#ifndef COLOPRESSO_INTERNAL_SIMD_H
#define COLOPRESSO_INTERNAL_SIMD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CPRES_USE_SIMD)
#if defined(__wasm_simd128__)
#define CPRES_SIMD_WASM128 1
#include <wasm_simd128.h>
#elif defined(_M_ARM64) || defined(_M_ARM)
#define CPRES_SIMD_NEON 1
#include <arm64_neon.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#define CPRES_SIMD_NEON 1
#include <arm_neon.h>
#elif defined(_M_IX86) || defined(_M_X64)
#define CPRES_SIMD_SSE41 1
#include <intrin.h>
#elif defined(__SSE4_1__)
#define CPRES_SIMD_SSE41 1
#include <smmintrin.h>
#elif defined(__SSE2__)
#define CPRES_SIMD_SSE2 1
#include <emmintrin.h>
#endif

#if defined(CPRES_SIMD_WASM128) || defined(CPRES_SIMD_SSE41) || defined(CPRES_SIMD_SSE2) || defined(CPRES_SIMD_NEON)
#define CPRES_SIMD_ENABLED 1
#endif
#endif

uint32_t simd_color_distance_sq_u32(uint32_t lhs, uint32_t rhs);

#ifdef __cplusplus
}
#endif

#endif /* COLOPRESSO_INTERNAL_SIMD_H */
