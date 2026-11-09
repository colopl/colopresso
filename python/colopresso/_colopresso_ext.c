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

#define PY_SSIZE_T_CLEAN
#define Py_LIMITED_API 0x030a0000 /* Python 3.10+ */

#include <Python.h>

#include <colopresso.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_FIELD(field, kind) {#field, offsetof(cpres_config_t, field), kind}
#define PROTECTED_COLORS_KEY      "pngx_protected_colors"
#define PROTECTED_COLORS_MAX      256

typedef enum {
    CONFIG_FIELD_INT,
    CONFIG_FIELD_FLOAT,
    CONFIG_FIELD_BOOL,
} config_field_kind_t;

typedef struct {
    const char *name;
    size_t offset;
    config_field_kind_t kind;
} config_field_t;

typedef struct {
    cpres_rgba_color_t *colors;
    int count;
} protected_colors_t;

typedef cpres_error_t (*encode_memory_func_t)(const uint8_t *png_data, size_t png_size,
                                              uint8_t **out_data, size_t *out_size,
                                              const cpres_config_t *config);

static const config_field_t kConfigFields[] = {
    CONFIG_FIELD(webp_quality, CONFIG_FIELD_FLOAT),
    CONFIG_FIELD(webp_lossless, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(webp_method, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_target_size, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_target_psnr, CONFIG_FIELD_FLOAT),
    CONFIG_FIELD(webp_segments, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_sns_strength, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_filter_strength, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_filter_sharpness, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_filter_type, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_autofilter, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(webp_alpha_compression, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(webp_alpha_filtering, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_alpha_quality, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_pass, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_preprocessing, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_partitions, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_partition_limit, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_emulate_jpeg_size, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(webp_thread_level, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_low_memory, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(webp_near_lossless, CONFIG_FIELD_INT),
    CONFIG_FIELD(webp_exact, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(webp_use_delta_palette, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(webp_use_sharp_yuv, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(avif_quality, CONFIG_FIELD_FLOAT),
    CONFIG_FIELD(avif_alpha_quality, CONFIG_FIELD_INT),
    CONFIG_FIELD(avif_lossless, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(avif_speed, CONFIG_FIELD_INT),
    CONFIG_FIELD(avif_threads, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_level, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_strip_safe, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(pngx_optimize_alpha, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(pngx_lossy_enable, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(pngx_lossy_type, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_lossy_max_colors, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_lossy_reduced_colors, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_lossy_reduced_bits_rgb, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_lossy_reduced_alpha_bits, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_lossy_quality_min, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_lossy_quality_max, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_lossy_speed, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_lossy_dither_level, CONFIG_FIELD_FLOAT),
    CONFIG_FIELD(pngx_saliency_map_enable, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(pngx_chroma_anchor_enable, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(pngx_adaptive_dither_enable, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(pngx_gradient_boost_enable, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(pngx_chroma_weight_enable, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(pngx_postprocess_smooth_enable, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(pngx_postprocess_smooth_importance_cutoff, CONFIG_FIELD_FLOAT),
    CONFIG_FIELD(pngx_palette256_gradient_profile_enable, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(pngx_palette256_gradient_dither_floor, CONFIG_FIELD_FLOAT),
    CONFIG_FIELD(pngx_palette256_alpha_bleed_enable, CONFIG_FIELD_BOOL),
    CONFIG_FIELD(pngx_palette256_alpha_bleed_max_distance, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_palette256_alpha_bleed_opaque_threshold, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_palette256_alpha_bleed_soft_limit, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_palette256_profile_opaque_ratio_threshold, CONFIG_FIELD_FLOAT),
    CONFIG_FIELD(pngx_palette256_profile_gradient_mean_max, CONFIG_FIELD_FLOAT),
    CONFIG_FIELD(pngx_palette256_profile_saturation_mean_max, CONFIG_FIELD_FLOAT),
    CONFIG_FIELD(pngx_palette256_tune_opaque_ratio_threshold, CONFIG_FIELD_FLOAT),
    CONFIG_FIELD(pngx_palette256_tune_gradient_mean_max, CONFIG_FIELD_FLOAT),
    CONFIG_FIELD(pngx_palette256_tune_saturation_mean_max, CONFIG_FIELD_FLOAT),
    CONFIG_FIELD(pngx_palette256_tune_speed_max, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_palette256_tune_quality_min_floor, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_palette256_tune_quality_max_target, CONFIG_FIELD_INT),
    CONFIG_FIELD(pngx_threads, CONFIG_FIELD_INT),
};

static PyObject *ColopressoError;

static inline bool key_equals(const char *key, Py_ssize_t key_len, const char *name)
{
    size_t name_len = strlen(name);

    return (size_t)key_len == name_len && memcmp(key, name, name_len) == 0;
}

static inline void free_protected_colors(protected_colors_t *pcolors)
{
    free(pcolors->colors);
    pcolors->colors = NULL;
    pcolors->count = 0;
}

static PyObject *raise_colopresso_error(cpres_error_t err)
{
    const char *msg = cpres_error_string(err);
    PyObject *exc;

    exc = PyObject_CallFunction(ColopressoError, "is", (int)err, msg ? msg : "Unknown error");
    if (exc) {
        PyErr_SetObject(ColopressoError, exc);
        Py_DECREF(exc);
    }
    return NULL;
}

static int read_color_component(PyObject *color, Py_ssize_t index, uint8_t *component)
{
    long value;

    value = PyLong_AsLong(PyTuple_GetItem(color, index));
    if (value == -1 && PyErr_Occurred()) {
        return -1;
    }

    if (value < 0 || value > 255) {
        PyErr_SetString(PyExc_ValueError, "Color component values must be 0-255");

        return -1;
    }

    *component = (uint8_t)value;

    return 0;
}

static int parse_protected_colors(PyObject *list, protected_colors_t *pcolors)
{
    PyObject *snapshot, *item;
    Py_ssize_t len, i;
    int status = -1;

    free_protected_colors(pcolors);

    if (list == NULL || list == Py_None) {
        return 0;
    }

    if (!PyList_Check(list)) {
        PyErr_SetString(PyExc_TypeError, "pngx_protected_colors must be a list");
        return -1;
    }

    /* A tuple of the items: the conversions below may run Python code that changes the list. */
    snapshot = PySequence_Tuple(list);
    if (!snapshot) {
        return -1;
    }

    len = PyTuple_Size(snapshot);
    if (len <= 0) {
        Py_DECREF(snapshot);

        return 0;
    }

    if (len > PROTECTED_COLORS_MAX) {
        PyErr_SetString(PyExc_ValueError, "pngx_protected_colors cannot exceed 256 colors");
        Py_DECREF(snapshot);

        return -1;
    }

    pcolors->colors = (cpres_rgba_color_t *)malloc(sizeof(cpres_rgba_color_t) * (size_t)len);
    if (!pcolors->colors) {
        PyErr_NoMemory();
        Py_DECREF(snapshot);

        return -1;
    }

    for (i = 0; i < len; ++i) {
        item = PyTuple_GetItem(snapshot, i);
        if (!PyTuple_Check(item) || PyTuple_Size(item) != 4) {
            PyErr_SetString(PyExc_TypeError,
                            "Each protected color must be a tuple of (r, g, b, a)");

            goto cleanup;
        }

        if (read_color_component(item, 0, &pcolors->colors[i].r) < 0 ||
            read_color_component(item, 1, &pcolors->colors[i].g) < 0 ||
            read_color_component(item, 2, &pcolors->colors[i].b) < 0 ||
            read_color_component(item, 3, &pcolors->colors[i].a) < 0) {
            goto cleanup;
        }
    }

    pcolors->count = (int)len;
    status = 0;

cleanup:
    if (status != 0) {
        free_protected_colors(pcolors);
    }

    Py_DECREF(snapshot);

    return status;
}

static int assign_config_field(const config_field_t *field, PyObject *value, cpres_config_t *config)
{
    long int_value;
    int truth, narrowed;
    bool flag;
    char *target = (char *)config + field->offset;
    double float_value;
    float narrowed_float;

    switch (field->kind) {
    case CONFIG_FIELD_INT:
        int_value = PyLong_AsLong(value);
        if (int_value == -1 && PyErr_Occurred()) {
            return -1;
        }

        if (int_value < INT_MIN || int_value > INT_MAX) {
            PyErr_Format(PyExc_OverflowError, "%s is out of range", field->name);

            return -1;
        }

        narrowed = (int)int_value;
        memcpy(target, &narrowed, sizeof(narrowed));

        return 0;
    case CONFIG_FIELD_FLOAT:
        float_value = PyFloat_AsDouble(value);
        if (float_value == -1.0 && PyErr_Occurred()) {
            return -1;
        }

        if (!isfinite(float_value) || float_value > FLT_MAX || float_value < -FLT_MAX) {
            PyErr_Format(PyExc_ValueError, "%s must be a finite number", field->name);

            return -1;
        }

        narrowed_float = (float)float_value;
        memcpy(target, &narrowed_float, sizeof(narrowed_float));

        return 0;
    case CONFIG_FIELD_BOOL:
        truth = PyObject_IsTrue(value);
        if (truth < 0) {
            return -1;
        }

        flag = truth != 0;
        memcpy(target, &flag, sizeof(flag));

        return 0;
    }

    return 0;
}

static int parse_config(PyObject *config_dict, cpres_config_t *config, protected_colors_t *pcolors)
{
    const char *key_str;
    PyObject *snapshot, *key, *value;
    Py_ssize_t pos = 0, key_len;
    int status = 0;
    size_t i;

    cpres_config_init_defaults(config);
    pcolors->colors = NULL;
    pcolors->count = 0;

    if (config_dict == NULL || config_dict == Py_None) {
        return 0;
    }

    if (!PyDict_Check(config_dict)) {
        PyErr_SetString(PyExc_TypeError, "config must be a dictionary");
        return -1;
    }

    /* A copy of the dictionary: the conversions below may run Python code that changes it. */
    snapshot = PyDict_Copy(config_dict);
    if (!snapshot) {
        return -1;
    }

    while (status == 0 && PyDict_Next(snapshot, &pos, &key, &value)) {
        key_str = PyUnicode_AsUTF8AndSize(key, &key_len);
        if (!key_str) {
            status = -1;

            break;
        }

        if (key_equals(key_str, key_len, PROTECTED_COLORS_KEY)) {
            status = parse_protected_colors(value, pcolors);
            config->pngx_protected_colors = pcolors->colors;
            config->pngx_protected_colors_count = pcolors->count;
            continue;
        }

        for (i = 0; i < sizeof(kConfigFields) / sizeof(kConfigFields[0]); ++i) {
            if (key_equals(key_str, key_len, kConfigFields[i].name)) {
                status = assign_config_field(&kConfigFields[i], value, config);

                break;
            }
        }
    }

    Py_DECREF(snapshot);

    if (status != 0) {
        free_protected_colors(pcolors);
        config->pngx_protected_colors = NULL;
        config->pngx_protected_colors_count = 0;
    }

    return status;
}

static PyObject *encode_with(encode_memory_func_t encode, PyObject *args, PyObject *kwargs)
{
    PyObject *config_dict = Py_None, *png_obj, *result;
    PyThreadState *thread_state;
    protected_colors_t pcolors = {NULL, 0};
    cpres_config_t config;
    cpres_error_t err;
    Py_ssize_t png_size;
    uint8_t *out_data = NULL;
    size_t out_size = 0;
    static char *kwlist[] = {"png_data", "config", NULL};
    char *png_data;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O", kwlist, &png_obj, &config_dict)) {
        return NULL;
    }

    if (!PyBytes_Check(png_obj)) {
        PyErr_SetString(PyExc_TypeError, "png_data must be bytes");
        return NULL;
    }

    if (PyBytes_AsStringAndSize(png_obj, &png_data, &png_size) < 0) {
        return NULL;
    }

    if (parse_config(config_dict, &config, &pcolors) < 0) {
        return NULL;
    }

    thread_state = PyEval_SaveThread();
    err = encode((const uint8_t *)png_data, (size_t)png_size, &out_data, &out_size, &config);
    PyEval_RestoreThread(thread_state);

    free_protected_colors(&pcolors);

    if (err != CPRES_OK) {
        return raise_colopresso_error(err);
    }

    if (!out_data) {
        return raise_colopresso_error(CPRES_ERROR_ENCODE_FAILED);
    }

    result = PyBytes_FromStringAndSize((const char *)out_data, (Py_ssize_t)out_size);
    cpres_free(out_data);
    return result;
}

static PyObject *py_encode_webp(PyObject *self, PyObject *args, PyObject *kwargs)
{
    (void)self;

    return encode_with(cpres_encode_webp_memory, args, kwargs);
}

static PyObject *py_encode_avif(PyObject *self, PyObject *args, PyObject *kwargs)
{
    (void)self;

    return encode_with(cpres_encode_avif_memory, args, kwargs);
}

static PyObject *py_encode_pngx(PyObject *self, PyObject *args, PyObject *kwargs)
{
    (void)self;

    return encode_with(cpres_encode_pngx_memory, args, kwargs);
}

static PyObject *py_get_version(PyObject *self, PyObject *Py_UNUSED(args))
{
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_version());
}

static PyObject *py_get_libwebp_version(PyObject *self, PyObject *Py_UNUSED(args))
{
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_libwebp_version());
}

static PyObject *py_get_libpng_version(PyObject *self, PyObject *Py_UNUSED(args))
{
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_libpng_version());
}

static PyObject *py_get_libavif_version(PyObject *self, PyObject *Py_UNUSED(args))
{
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_libavif_version());
}

static PyObject *py_get_pngx_oxipng_version(PyObject *self, PyObject *Py_UNUSED(args))
{
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_pngx_oxipng_version());
}

static PyObject *py_get_pngx_libimagequant_version(PyObject *self, PyObject *Py_UNUSED(args))
{
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_pngx_libimagequant_version());
}

static PyObject *py_get_buildtime(PyObject *self, PyObject *Py_UNUSED(args))
{
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_buildtime());
}

static PyObject *py_get_compiler_version_string(PyObject *self, PyObject *Py_UNUSED(args))
{
    const char *s;
    (void)self;
    s = cpres_get_compiler_version_string();
    return PyUnicode_FromString(s ? s : "");
}

static PyObject *py_get_rust_version_string(PyObject *self, PyObject *Py_UNUSED(args))
{
    const char *s;
    (void)self;
    s = cpres_get_rust_version_string();
    return PyUnicode_FromString(s ? s : "");
}

static PyObject *py_is_threads_enabled(PyObject *self, PyObject *Py_UNUSED(args))
{
    (void)self;
    return PyBool_FromLong(cpres_is_threads_enabled());
}

static PyObject *py_get_default_thread_count(PyObject *self, PyObject *Py_UNUSED(args))
{
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_default_thread_count());
}

static PyObject *py_get_max_thread_count(PyObject *self, PyObject *Py_UNUSED(args))
{
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_max_thread_count());
}

static PyMethodDef colopresso_methods[] = {
    {"encode_webp", (PyCFunction)py_encode_webp, METH_VARARGS | METH_KEYWORDS,
     "Encode PNG data to WebP format.\n\n"
     "Args:\n"
     "    png_data: Raw PNG file data (bytes)\n"
     "    config: Optional configuration dictionary\n\n"
     "Returns:\n"
     "    WebP encoded data (bytes)"},
    {"encode_avif", (PyCFunction)py_encode_avif, METH_VARARGS | METH_KEYWORDS,
     "Encode PNG data to AVIF format.\n\n"
     "Args:\n"
     "    png_data: Raw PNG file data (bytes)\n"
     "    config: Optional configuration dictionary\n\n"
     "Returns:\n"
     "    AVIF encoded data (bytes)"},
    {"encode_pngx", (PyCFunction)py_encode_pngx, METH_VARARGS | METH_KEYWORDS,
     "Optimize PNG data using PNGX encoder.\n\n"
     "Args:\n"
     "    png_data: Raw PNG file data (bytes)\n"
     "    config: Optional configuration dictionary\n\n"
     "Returns:\n"
     "    Optimized PNG data (bytes)"},
    {"get_version", py_get_version, METH_NOARGS, "Get colopresso version number"},
    {"get_libwebp_version", py_get_libwebp_version, METH_NOARGS, "Get libwebp version number"},
    {"get_libpng_version", py_get_libpng_version, METH_NOARGS, "Get libpng version number"},
    {"get_libavif_version", py_get_libavif_version, METH_NOARGS, "Get libavif version number"},
    {"get_pngx_oxipng_version", py_get_pngx_oxipng_version, METH_NOARGS,
     "Get oxipng version number"},
    {"get_pngx_libimagequant_version", py_get_pngx_libimagequant_version, METH_NOARGS,
     "Get libimagequant version number"},
    {"get_buildtime", py_get_buildtime, METH_NOARGS, "Get build timestamp"},
    {"get_compiler_version_string", py_get_compiler_version_string, METH_NOARGS,
     "Get compiler version string"},
    {"get_rust_version_string", py_get_rust_version_string, METH_NOARGS, "Get Rust version string"},
    {"is_threads_enabled", py_is_threads_enabled, METH_NOARGS, "Check if threading is enabled"},
    {"get_default_thread_count", py_get_default_thread_count, METH_NOARGS,
     "Get default thread count"},
    {"get_max_thread_count", py_get_max_thread_count, METH_NOARGS, "Get maximum thread count"},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef colopresso_module = {PyModuleDef_HEAD_INIT,
                                               "_colopresso",
                                               "colopresso image compression library",
                                               -1,
                                               colopresso_methods,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL};

PyMODINIT_FUNC PyInit__colopresso(void)
{
    PyObject *m;

    m = PyModule_Create(&colopresso_module);
    if (m == NULL) {
        return NULL;
    }

    ColopressoError = PyErr_NewException("_colopresso.ColopressoError", NULL, NULL);
    if (ColopressoError == NULL) {
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(ColopressoError);
    if (PyModule_AddObject(m, "ColopressoError", ColopressoError) < 0) {
        Py_DECREF(ColopressoError);
        Py_CLEAR(ColopressoError);
        Py_DECREF(m);
        return NULL;
    }

    if (PyModule_AddIntConstant(m, "PNGX_LOSSY_TYPE_PALETTE256",
                                COLOPRESSO_PNGX_LOSSY_TYPE_PALETTE256) < 0 ||
        PyModule_AddIntConstant(m, "PNGX_LOSSY_TYPE_LIMITED_RGBA4444",
                                COLOPRESSO_PNGX_LOSSY_TYPE_LIMITED_RGBA4444) < 0 ||
        PyModule_AddIntConstant(m, "PNGX_LOSSY_TYPE_REDUCED_RGBA32",
                                COLOPRESSO_PNGX_LOSSY_TYPE_REDUCED_RGBA32) < 0) {
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
