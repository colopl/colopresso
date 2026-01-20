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
#define Py_LIMITED_API 0x030a0000  /* Python 3.10+ */

#include <Python.h>

#include <colopresso.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static PyObject *ColopressoError;

typedef struct {
    cpres_rgba_color_t *colors;
    int count;
} protected_colors_t;

static inline char *get_utf8_string(PyObject *obj) {
    PyObject *bytes;
    Py_ssize_t len;
    char *result, *str;

    bytes = PyUnicode_AsEncodedString(obj, "utf-8", "strict");
    if (!bytes) return NULL;

    if (PyBytes_AsStringAndSize(bytes, &str, &len) < 0) {
        Py_DECREF(bytes);
        return NULL;
    }

    result = (char *)malloc((size_t)len + 1);
    if (result) {
        memcpy(result, str, (size_t)len);
        result[len] = '\0';
    }

    Py_DECREF(bytes);
    return result;
}

static PyObject *raise_colopresso_error(cpres_error_t err) {
    const char *msg = cpres_error_string(err);
    PyObject *args = Py_BuildValue("(is)", (int)err, msg ? msg : "Unknown error");
    if (args) {
        PyErr_SetObject(ColopressoError, args);
        Py_DECREF(args);
    }
    return NULL;
}

static int parse_protected_colors(PyObject *list, protected_colors_t *pcolors) {
    Py_ssize_t len, i;
    PyObject *item;
    long r, g, b, a;

    pcolors->colors = NULL;
    pcolors->count = 0;

    if (list == NULL || list == Py_None) {
        return 0;
    }

    if (!PyList_Check(list)) {
        PyErr_SetString(PyExc_TypeError, "pngx_protected_colors must be a list");
        return -1;
    }

    len = PyList_Size(list);
    if (len <= 0) {
        return 0;
    }
    if (len > 256) {
        PyErr_SetString(PyExc_ValueError, "pngx_protected_colors cannot exceed 256 colors");
        return -1;
    }

    pcolors->colors = (cpres_rgba_color_t *)malloc(sizeof(cpres_rgba_color_t) * (size_t)len);
    if (!pcolors->colors) {
        PyErr_NoMemory();
        return -1;
    }

    for (i = 0; i < len; i++) {
        item = PyList_GetItem(list, i);  /* borrowed reference */
        if (!PyTuple_Check(item) || PyTuple_Size(item) != 4) {
            free(pcolors->colors);
            pcolors->colors = NULL;
            PyErr_SetString(PyExc_TypeError, "Each protected color must be a tuple of (r, g, b, a)");
            return -1;
        }

        r = PyLong_AsLong(PyTuple_GetItem(item, 0));
        g = PyLong_AsLong(PyTuple_GetItem(item, 1));
        b = PyLong_AsLong(PyTuple_GetItem(item, 2));
        a = PyLong_AsLong(PyTuple_GetItem(item, 3));

        if (PyErr_Occurred()) {
            free(pcolors->colors);
            pcolors->colors = NULL;
            return -1;
        }

        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 || a > 255) {
            free(pcolors->colors);
            pcolors->colors = NULL;
            PyErr_SetString(PyExc_ValueError, "Color component values must be 0-255");
            return -1;
        }

        pcolors->colors[i].r = (uint8_t)r;
        pcolors->colors[i].g = (uint8_t)g;
        pcolors->colors[i].b = (uint8_t)b;
        pcolors->colors[i].a = (uint8_t)a;
    }

    pcolors->count = (int)len;
    return 0;
}

static void free_protected_colors(protected_colors_t *pcolors) {
    if (pcolors->colors) {
        free(pcolors->colors);
        pcolors->colors = NULL;
    }
    pcolors->count = 0;
}

static int parse_config(PyObject *config_dict, cpres_config_t *config, protected_colors_t *pcolors) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;

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

    while (PyDict_Next(config_dict, &pos, &key, &value)) {
        char *key_str = get_utf8_string(key);
        if (!key_str) return -1;

        /* WebP */
        if (strcmp(key_str, "webp_quality") == 0) {
            config->webp_quality = (float)PyFloat_AsDouble(value);
        } else if (strcmp(key_str, "webp_lossless") == 0) {
            config->webp_lossless = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "webp_method") == 0) {
            config->webp_method = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_target_size") == 0) {
            config->webp_target_size = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_target_psnr") == 0) {
            config->webp_target_psnr = (float)PyFloat_AsDouble(value);
        } else if (strcmp(key_str, "webp_segments") == 0) {
            config->webp_segments = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_sns_strength") == 0) {
            config->webp_sns_strength = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_filter_strength") == 0) {
            config->webp_filter_strength = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_filter_sharpness") == 0) {
            config->webp_filter_sharpness = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_filter_type") == 0) {
            config->webp_filter_type = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_autofilter") == 0) {
            config->webp_autofilter = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "webp_alpha_compression") == 0) {
            config->webp_alpha_compression = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "webp_alpha_filtering") == 0) {
            config->webp_alpha_filtering = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_alpha_quality") == 0) {
            config->webp_alpha_quality = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_pass") == 0) {
            config->webp_pass = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_preprocessing") == 0) {
            config->webp_preprocessing = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_partitions") == 0) {
            config->webp_partitions = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_partition_limit") == 0) {
            config->webp_partition_limit = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_emulate_jpeg_size") == 0) {
            config->webp_emulate_jpeg_size = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "webp_thread_level") == 0) {
            config->webp_thread_level = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_low_memory") == 0) {
            config->webp_low_memory = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "webp_near_lossless") == 0) {
            config->webp_near_lossless = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "webp_exact") == 0) {
            config->webp_exact = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "webp_use_delta_palette") == 0) {
            config->webp_use_delta_palette = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "webp_use_sharp_yuv") == 0) {
            config->webp_use_sharp_yuv = PyObject_IsTrue(value);
        }

        /* AVIF */
        else if (strcmp(key_str, "avif_quality") == 0) {
            config->avif_quality = (float)PyFloat_AsDouble(value);
        } else if (strcmp(key_str, "avif_alpha_quality") == 0) {
            config->avif_alpha_quality = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "avif_lossless") == 0) {
            config->avif_lossless = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "avif_speed") == 0) {
            config->avif_speed = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "avif_threads") == 0) {
            config->avif_threads = (int)PyLong_AsLong(value);
        }

        /* PNGX (PNG) */
        else if (strcmp(key_str, "pngx_level") == 0) {
            config->pngx_level = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_strip_safe") == 0) {
            config->pngx_strip_safe = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "pngx_optimize_alpha") == 0) {
            config->pngx_optimize_alpha = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "pngx_lossy_enable") == 0) {
            config->pngx_lossy_enable = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "pngx_lossy_type") == 0) {
            config->pngx_lossy_type = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_lossy_max_colors") == 0) {
            config->pngx_lossy_max_colors = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_lossy_reduced_colors") == 0) {
            config->pngx_lossy_reduced_colors = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_lossy_reduced_bits_rgb") == 0) {
            config->pngx_lossy_reduced_bits_rgb = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_lossy_reduced_alpha_bits") == 0) {
            config->pngx_lossy_reduced_alpha_bits = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_lossy_quality_min") == 0) {
            config->pngx_lossy_quality_min = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_lossy_quality_max") == 0) {
            config->pngx_lossy_quality_max = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_lossy_speed") == 0) {
            config->pngx_lossy_speed = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_lossy_dither_level") == 0) {
            config->pngx_lossy_dither_level = (float)PyFloat_AsDouble(value);
        } else if (strcmp(key_str, "pngx_saliency_map_enable") == 0) {
            config->pngx_saliency_map_enable = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "pngx_chroma_anchor_enable") == 0) {
            config->pngx_chroma_anchor_enable = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "pngx_adaptive_dither_enable") == 0) {
            config->pngx_adaptive_dither_enable = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "pngx_gradient_boost_enable") == 0) {
            config->pngx_gradient_boost_enable = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "pngx_chroma_weight_enable") == 0) {
            config->pngx_chroma_weight_enable = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "pngx_postprocess_smooth_enable") == 0) {
            config->pngx_postprocess_smooth_enable = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "pngx_postprocess_smooth_importance_cutoff") == 0) {
            config->pngx_postprocess_smooth_importance_cutoff = (float)PyFloat_AsDouble(value);
        } else if (strcmp(key_str, "pngx_palette256_gradient_profile_enable") == 0) {
            config->pngx_palette256_gradient_profile_enable = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "pngx_palette256_gradient_dither_floor") == 0) {
            config->pngx_palette256_gradient_dither_floor = (float)PyFloat_AsDouble(value);
        } else if (strcmp(key_str, "pngx_palette256_alpha_bleed_enable") == 0) {
            config->pngx_palette256_alpha_bleed_enable = PyObject_IsTrue(value);
        } else if (strcmp(key_str, "pngx_palette256_alpha_bleed_max_distance") == 0) {
            config->pngx_palette256_alpha_bleed_max_distance = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_palette256_alpha_bleed_opaque_threshold") == 0) {
            config->pngx_palette256_alpha_bleed_opaque_threshold = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_palette256_alpha_bleed_soft_limit") == 0) {
            config->pngx_palette256_alpha_bleed_soft_limit = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_palette256_profile_opaque_ratio_threshold") == 0) {
            config->pngx_palette256_profile_opaque_ratio_threshold = (float)PyFloat_AsDouble(value);
        } else if (strcmp(key_str, "pngx_palette256_profile_gradient_mean_max") == 0) {
            config->pngx_palette256_profile_gradient_mean_max = (float)PyFloat_AsDouble(value);
        } else if (strcmp(key_str, "pngx_palette256_profile_saturation_mean_max") == 0) {
            config->pngx_palette256_profile_saturation_mean_max = (float)PyFloat_AsDouble(value);
        } else if (strcmp(key_str, "pngx_palette256_tune_opaque_ratio_threshold") == 0) {
            config->pngx_palette256_tune_opaque_ratio_threshold = (float)PyFloat_AsDouble(value);
        } else if (strcmp(key_str, "pngx_palette256_tune_gradient_mean_max") == 0) {
            config->pngx_palette256_tune_gradient_mean_max = (float)PyFloat_AsDouble(value);
        } else if (strcmp(key_str, "pngx_palette256_tune_saturation_mean_max") == 0) {
            config->pngx_palette256_tune_saturation_mean_max = (float)PyFloat_AsDouble(value);
        } else if (strcmp(key_str, "pngx_palette256_tune_speed_max") == 0) {
            config->pngx_palette256_tune_speed_max = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_palette256_tune_quality_min_floor") == 0) {
            config->pngx_palette256_tune_quality_min_floor = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_palette256_tune_quality_max_target") == 0) {
            config->pngx_palette256_tune_quality_max_target = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_threads") == 0) {
            config->pngx_threads = (int)PyLong_AsLong(value);
        } else if (strcmp(key_str, "pngx_protected_colors") == 0) {
            free(key_str);
            if (parse_protected_colors(value, pcolors) < 0) {
                return -1;
            }
            config->pngx_protected_colors = pcolors->colors;
            config->pngx_protected_colors_count = pcolors->count;
            continue;
        }

        free(key_str);

        if (PyErr_Occurred()) {
            return -1;
        }
    }

    return 0;
}

static PyObject *py_encode_webp(PyObject *self, PyObject *args, PyObject *kwargs) {
    static char *kwlist[] = {"png_data", "config", NULL};
    PyObject *config_dict = Py_None, *result, *png_obj;
    Py_ssize_t png_size;
    cpres_config_t config;
    cpres_error_t err;
    protected_colors_t pcolors = {NULL, 0};
    uint8_t *out_data = NULL;
    size_t out_size = 0;
    char *png_data;

    (void)self;

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

    Py_BEGIN_ALLOW_THREADS
    err = cpres_encode_webp_memory((const uint8_t *)png_data, (size_t)png_size,
                                   &out_data, &out_size, &config);
    Py_END_ALLOW_THREADS

    free_protected_colors(&pcolors);

    if (err != CPRES_OK) {
        return raise_colopresso_error(err);
    }

    result = PyBytes_FromStringAndSize((const char *)out_data, (Py_ssize_t)out_size);
    cpres_free(out_data);
    return result;
}

static PyObject *py_encode_avif(PyObject *self, PyObject *args, PyObject *kwargs) {
    static char *kwlist[] = {"png_data", "config", NULL};
    PyObject *config_dict = Py_None, *png_obj, *result;
    Py_ssize_t png_size;
    cpres_config_t config;
    cpres_error_t err;
    protected_colors_t pcolors = {NULL, 0};
    uint8_t *out_data = NULL;
    size_t out_size = 0;
    char *png_data;

    (void)self;

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

    Py_BEGIN_ALLOW_THREADS
    err = cpres_encode_avif_memory((const uint8_t *)png_data, (size_t)png_size,
                                   &out_data, &out_size, &config);
    Py_END_ALLOW_THREADS

    free_protected_colors(&pcolors);

    if (err != CPRES_OK) {
        return raise_colopresso_error(err);
    }

    result = PyBytes_FromStringAndSize((const char *)out_data, (Py_ssize_t)out_size);
    cpres_free(out_data);
    return result;
}

static PyObject *py_encode_pngx(PyObject *self, PyObject *args, PyObject *kwargs) {
    static char *kwlist[] = {"png_data", "config", NULL};
    PyObject *config_dict = Py_None, *png_obj, *result;
    Py_ssize_t png_size;
    cpres_config_t config;
    cpres_error_t err;
    protected_colors_t pcolors = {NULL, 0};
    uint8_t *out_data = NULL;
    size_t out_size = 0;
    char *png_data;

    (void)self;

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

    Py_BEGIN_ALLOW_THREADS
    err = cpres_encode_pngx_memory((const uint8_t *)png_data, (size_t)png_size,
                                   &out_data, &out_size, &config);
    Py_END_ALLOW_THREADS

    free_protected_colors(&pcolors);

    if (err != CPRES_OK) {
        return raise_colopresso_error(err);
    }

    result = PyBytes_FromStringAndSize((const char *)out_data, (Py_ssize_t)out_size);
    cpres_free(out_data);
    return result;
}

static PyObject *py_get_version(PyObject *self, PyObject *Py_UNUSED(args)) {
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_version());
}

static PyObject *py_get_libwebp_version(PyObject *self, PyObject *Py_UNUSED(args)) {
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_libwebp_version());
}

static PyObject *py_get_libpng_version(PyObject *self, PyObject *Py_UNUSED(args)) {
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_libpng_version());
}

static PyObject *py_get_libavif_version(PyObject *self, PyObject *Py_UNUSED(args)) {
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_libavif_version());
}

static PyObject *py_get_pngx_oxipng_version(PyObject *self, PyObject *Py_UNUSED(args)) {
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_pngx_oxipng_version());
}

static PyObject *py_get_pngx_libimagequant_version(PyObject *self, PyObject *Py_UNUSED(args)) {
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_pngx_libimagequant_version());
}

static PyObject *py_get_buildtime(PyObject *self, PyObject *Py_UNUSED(args)) {
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_buildtime());
}

static PyObject *py_get_compiler_version_string(PyObject *self, PyObject *Py_UNUSED(args)) {
    const char *s;
    (void)self;
    s = cpres_get_compiler_version_string();
    return PyUnicode_FromString(s ? s : "");
}

static PyObject *py_get_rust_version_string(PyObject *self, PyObject *Py_UNUSED(args)) {
    const char *s;
    (void)self;
    s = cpres_get_rust_version_string();
    return PyUnicode_FromString(s ? s : "");
}

static PyObject *py_is_threads_enabled(PyObject *self, PyObject *Py_UNUSED(args)) {
    (void)self;
    return PyBool_FromLong(cpres_is_threads_enabled());
}

static PyObject *py_get_default_thread_count(PyObject *self, PyObject *Py_UNUSED(args)) {
    (void)self;
    return PyLong_FromUnsignedLong(cpres_get_default_thread_count());
}

static PyObject *py_get_max_thread_count(PyObject *self, PyObject *Py_UNUSED(args)) {
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
    {"get_pngx_oxipng_version", py_get_pngx_oxipng_version, METH_NOARGS, "Get oxipng version number"},
    {"get_pngx_libimagequant_version", py_get_pngx_libimagequant_version, METH_NOARGS, "Get libimagequant version number"},
    {"get_buildtime", py_get_buildtime, METH_NOARGS, "Get build timestamp"},
    {"get_compiler_version_string", py_get_compiler_version_string, METH_NOARGS, "Get compiler version string"},
    {"get_rust_version_string", py_get_rust_version_string, METH_NOARGS, "Get Rust version string"},
    {"is_threads_enabled", py_is_threads_enabled, METH_NOARGS, "Check if threading is enabled"},
    {"get_default_thread_count", py_get_default_thread_count, METH_NOARGS, "Get default thread count"},
    {"get_max_thread_count", py_get_max_thread_count, METH_NOARGS, "Get maximum thread count"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef colopresso_module = {
    PyModuleDef_HEAD_INIT,
    "_colopresso",
    "colopresso image compression library",
    -1,
    colopresso_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit__colopresso(void) {
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
        Py_DECREF(m);
        return NULL;
    }

    if (PyModule_AddIntConstant(m, "PNGX_LOSSY_TYPE_PALETTE256", COLOPRESSO_PNGX_LOSSY_TYPE_PALETTE256) < 0 ||
        PyModule_AddIntConstant(m, "PNGX_LOSSY_TYPE_LIMITED_RGBA4444", COLOPRESSO_PNGX_LOSSY_TYPE_LIMITED_RGBA4444) < 0 ||
        PyModule_AddIntConstant(m, "PNGX_LOSSY_TYPE_REDUCED_RGBA32", COLOPRESSO_PNGX_LOSSY_TYPE_REDUCED_RGBA32) < 0) {
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
