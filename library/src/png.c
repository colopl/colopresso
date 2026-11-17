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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#include <colopresso.h>

#include "internal/log.h"
#include "internal/png.h"

#define PNG_DECODE_MAX_DIMENSION 65536u
#define PNG_DECODE_MAX_PIXELS    ((uint64_t)1 << 28)
#define PNG_DECODE_CHANNELS      4u
#define PNG_SIGNATURE_SIZE       8u
#define PNG_IHDR_DATA_SIZE       13u

typedef struct {
    const uint8_t *data;
    size_t size;
    size_t pos;
} png_memory_reader_t;

/* Buffers of one decode. They live in the caller of png_decode_guarded() so that they keep
 * their values when libpng longjmps out of png_read_rgba_rows(). */
typedef struct {
    png_bytep *row_pointers;
    uint8_t *rgba;
    png_uint_32 width;
    png_uint_32 height;
} png_decode_state_t;

static inline cpres_error_t png_decode_take(png_decode_state_t *state, cpres_error_t result,
                                            uint8_t **rgba_data, png_uint_32 *width,
                                            png_uint_32 *height)
{
    free(state->row_pointers);
    state->row_pointers = NULL;

    if (result != CPRES_OK) {
        free(state->rgba);
        state->rgba = NULL;

        return result;
    }

    *rgba_data = state->rgba;
    *width = state->width;
    *height = state->height;
    state->rgba = NULL;

    return CPRES_OK;
}

/* 2^28 pixels (16384 x 16384) is the largest image libavif and libwebp accept as well; it bounds
 * the RGBA buffer and every working copy the encoders make from it. */
static inline bool png_dimensions_within_limits(png_uint_32 width, png_uint_32 height)
{
    return width > 0 && height > 0 && width <= PNG_DECODE_MAX_DIMENSION &&
           height <= PNG_DECODE_MAX_DIMENSION &&
           (uint64_t)width * (uint64_t)height <= PNG_DECODE_MAX_PIXELS;
}

static void png_read_from_memory(png_structp png_ptr, png_bytep data, png_size_t length)
{
    png_memory_reader_t *reader = (png_memory_reader_t *)png_get_io_ptr(png_ptr);

    if (length > reader->size - reader->pos) {
        colopresso_log(CPRES_LOG_LEVEL_ERROR, "Attempted to read past end of PNG data");
        png_error(png_ptr, "Read past end of data");
    }

    memcpy(data, reader->data + reader->pos, length);

    reader->pos += length;
}

static bool png_create_read(png_structp *png, png_infop *info)
{
    *png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!*png) {
        return false;
    }

    *info = png_create_info_struct(*png);
    if (!*info) {
        png_destroy_read_struct(png, NULL, NULL);

        return false;
    }

    return true;
}

static cpres_error_t png_read_rgba_rows(png_structp png, png_infop info, png_decode_state_t *state)
{
    png_byte color_type, bit_depth;
    png_uint_32 y;
    size_t row_bytes;

    png_read_info(png, info);

    state->width = png_get_image_width(png, info);
    state->height = png_get_image_height(png, info);
    color_type = png_get_color_type(png, info);
    bit_depth = png_get_bit_depth(png, info);

    if (!png_dimensions_within_limits(state->width, state->height)) {
        colopresso_log(CPRES_LOG_LEVEL_ERROR, "PNG dimensions %ux%u exceed the supported size",
                       (unsigned int)state->width, (unsigned int)state->height);

        return CPRES_ERROR_INVALID_PNG;
    }

    if (bit_depth == 16) {
        png_set_strip_16(png);
    }

    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }

    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }

    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }

    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    row_bytes = png_get_rowbytes(png, info);
    if (row_bytes != (size_t)state->width * PNG_DECODE_CHANNELS) {
        colopresso_log(CPRES_LOG_LEVEL_ERROR, "Unexpected PNG row size after RGBA conversion");

        return CPRES_ERROR_INVALID_PNG;
    }

    if (row_bytes > SIZE_MAX / (size_t)state->height ||
        (size_t)state->height > SIZE_MAX / sizeof(png_bytep)) {
        colopresso_log(CPRES_LOG_LEVEL_ERROR, "Integer overflow detected: image too large");

        return CPRES_ERROR_OUT_OF_MEMORY;
    }

    state->rgba = (uint8_t *)malloc(row_bytes * (size_t)state->height);
    if (!state->rgba) {
        return CPRES_ERROR_OUT_OF_MEMORY;
    }

    state->row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * (size_t)state->height);
    if (!state->row_pointers) {
        return CPRES_ERROR_OUT_OF_MEMORY;
    }

    for (y = 0; y < state->height; ++y) {
        state->row_pointers[y] = state->rgba + (size_t)y * row_bytes;
    }

    png_read_image(png, state->row_pointers);

    return CPRES_OK;
}

static cpres_error_t png_decode_guarded(png_structp png, png_infop info, png_decode_state_t *state)
{
    if (setjmp(png_jmpbuf(png)) != 0) {
        return CPRES_ERROR_INVALID_PNG;
    }

    return png_read_rgba_rows(png, info, state);
}

extern cpres_error_t png_decode_from_memory(const uint8_t *png_data, size_t png_size,
                                            uint8_t **rgba_data, png_uint_32 *width,
                                            png_uint_32 *height)
{
    png_memory_reader_t reader = {0};
    png_decode_state_t state = {0};
    png_structp png;
    png_infop info;
    cpres_error_t result;

    if (!png_data || png_size < 8 || !rgba_data || !width || !height) {
        return CPRES_ERROR_INVALID_PARAMETER;
    }

    *rgba_data = NULL;

    if (png_sig_cmp(png_data, 0, 8) != 0) {
        return CPRES_ERROR_INVALID_PNG;
    }

    if (!png_create_read(&png, &info)) {
        return CPRES_ERROR_OUT_OF_MEMORY;
    }

    reader.data = png_data;
    reader.size = png_size;
    reader.pos = 0;
    png_set_read_fn(png, &reader, png_read_from_memory);

    result = png_decode_guarded(png, info, &state);
    png_destroy_read_struct(&png, &info, NULL);

    return png_decode_take(&state, result, rgba_data, width, height);
}

#if COLOPRESSO_WITH_FILE_OPS
extern cpres_error_t png_decode_from_file(const char *filename, uint8_t **rgba_data,
                                          png_uint_32 *width, png_uint_32 *height)
{
    png_decode_state_t state = {0};
    png_structp png;
    png_infop info;
    cpres_error_t result;
    FILE *fp;

    if (!filename || !rgba_data || !width || !height) {
        return CPRES_ERROR_INVALID_PARAMETER;
    }

    *rgba_data = NULL;

    fp = fopen(filename, "rb");
    if (!fp) {
        return CPRES_ERROR_FILE_NOT_FOUND;
    }

    if (!png_create_read(&png, &info)) {
        fclose(fp);

        return CPRES_ERROR_OUT_OF_MEMORY;
    }

    png_init_io(png, fp);

    result = png_decode_guarded(png, info, &state);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);

    return png_decode_take(&state, result, rgba_data, width, height);
}
#endif /* COLOPRESSO_WITH_FILE_OPS */

extern bool png_header_within_limits(const uint8_t *png_data, size_t png_size)
{
    uint32_t length;
    size_t offset;

    if (!png_data || png_size < PNG_SIGNATURE_SIZE + 8u + PNG_IHDR_DATA_SIZE ||
        png_sig_cmp(png_data, 0, PNG_SIGNATURE_SIZE) != 0) {
        return false;
    }

    if (png_get_uint_32(png_data + PNG_SIGNATURE_SIZE) != PNG_IHDR_DATA_SIZE ||
        memcmp(png_data + PNG_SIGNATURE_SIZE + 4u, "IHDR", 4) != 0) {
        return false;
    }

    if (!png_dimensions_within_limits(png_get_uint_32(png_data + PNG_SIGNATURE_SIZE + 8u),
                                      png_get_uint_32(png_data + PNG_SIGNATURE_SIZE + 12u))) {
        return false;
    }

    /* Chunk lengths above 2^31 - 1 are invalid; offset arithmetic in other parsers (oxipng on
     * 32-bit targets) can wrap on them. A truncated last chunk is left to the decoders. */
    offset = PNG_SIGNATURE_SIZE;
    while (png_size - offset >= 12u) {
        length = png_get_uint_32(png_data + offset);
        if (length > PNG_UINT_31_MAX) {
            return false;
        }

        if (length > png_size - offset - 12u) {
            break;
        }

        offset += 12u + (size_t)length;
    }

    return true;
}
