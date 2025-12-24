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

#include "internal/simd.h"

#if defined(CPRES_SIMD_SSE41) || defined(CPRES_SIMD_SSE2)

static inline uint32_t color_distance_sq_sse(uint32_t lhs, uint32_t rhs) {
    __m128i a = _mm_cvtsi32_si128((int)lhs);
    __m128i b = _mm_cvtsi32_si128((int)rhs);
    __m128i a16, b16, diff, diff_sq;
    int result;

    a16 = _mm_unpacklo_epi8(a, _mm_setzero_si128());
    b16 = _mm_unpacklo_epi8(b, _mm_setzero_si128());

    diff = _mm_sub_epi16(a16, b16);

    diff_sq = _mm_madd_epi16(diff, diff);

    diff_sq = _mm_add_epi32(diff_sq, _mm_srli_si128(diff_sq, 8));
    diff_sq = _mm_add_epi32(diff_sq, _mm_srli_si128(diff_sq, 4));

    result = _mm_cvtsi128_si32(diff_sq);
    return (uint32_t)result;
}

#elif defined(CPRES_SIMD_NEON)

static inline uint32_t color_distance_sq_neon(uint32_t lhs, uint32_t rhs) {
    uint8x8_t a = vreinterpret_u8_u32(vdup_n_u32(lhs)),
              b = vreinterpret_u8_u32(vdup_n_u32(rhs));
    int16x4_t a16 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(a))),
              b16 = vreinterpret_s16_u16(vget_low_u16(vmovl_u8(b))),
              diff = vsub_s16(a16, b16);
    int32x4_t diff_sq = vmull_s16(diff, diff);
    int32_t result = vgetq_lane_s32(diff_sq, 0) + vgetq_lane_s32(diff_sq, 1) +
                     vgetq_lane_s32(diff_sq, 2) + vgetq_lane_s32(diff_sq, 3);

    return (uint32_t)result;
}

#elif defined(CPRES_SIMD_WASM128)

static inline uint32_t color_distance_sq_wasm(uint32_t lhs, uint32_t rhs) {
    v128_t a = wasm_i32x4_splat((int32_t)lhs),
           b = wasm_i32x4_splat((int32_t)rhs),
           a16 = wasm_u16x8_extend_low_u8x16(a),
           b16 = wasm_u16x8_extend_low_u8x16(b),
           diff = wasm_i16x8_sub(a16, b16),
           diff_sq = wasm_i32x4_extmul_low_i16x8(diff, diff);
    int32_t result = wasm_i32x4_extract_lane(diff_sq, 0) +
                     wasm_i32x4_extract_lane(diff_sq, 1) +
                     wasm_i32x4_extract_lane(diff_sq, 2) +
                     wasm_i32x4_extract_lane(diff_sq, 3);

    return (uint32_t)result;
}

#else

static inline uint32_t color_distance_sq_scalar(uint32_t lhs, uint32_t rhs) {
    int32_t r1 = (int32_t)(lhs & 0xFF),
            g1 = (int32_t)((lhs >> 8) & 0xFF),
            b1 = (int32_t)((lhs >> 16) & 0xFF),
            a1 = (int32_t)((lhs >> 24) & 0xFF),
            r2 = (int32_t)(rhs & 0xFF),
            g2 = (int32_t)((rhs >> 8) & 0xFF),
            b2 = (int32_t)((rhs >> 16) & 0xFF),
            a2 = (int32_t)((rhs >> 24) & 0xFF),
            dr = r1 - r2,
            dg = g1 - g2,
            db = b1 - b2,
            da = a1 - a2;
    
    return (uint32_t)(dr*dr + dg*dg + db*db + da*da);
}

#endif

uint32_t simd_color_distance_sq_u32(uint32_t lhs, uint32_t rhs) {
#if defined(CPRES_SIMD_WASM128)
    return color_distance_sq_wasm(lhs, rhs);
#elif defined(CPRES_SIMD_SSE41) || defined(CPRES_SIMD_SSE2)
    return color_distance_sq_sse(lhs, rhs);
#elif defined(CPRES_SIMD_NEON)
    return color_distance_sq_neon(lhs, rhs);
#else
    return color_distance_sq_scalar(lhs, rhs);
#endif
}
