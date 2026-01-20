# Colopresso Python Bindings

Python bindings for the colopresso image compression library. Encode PNG images to WebP, AVIF, or optimized PNG (PNGX) formats.

## Table of Contents

- [Installation](#installation)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
  - [Encoding Functions](#encoding-functions)
  - [Config Class](#config-class)
  - [PngxLossyType Enum](#pngxlossytype-enum)
  - [Configuration Parameters](#configuration-parameters)
  - [Utility Functions](#utility-functions)
  - [Exception Classes](#exception-classes)
- [Examples](#examples)
- [Building Wheels](#building-wheels)

## Installation

### From PyPI

```bash
pip install "colopresso"
```

### From source (requires CMake, Rust, and C compiler)

```bash
cd python
pip install .
```

### Development installation

```bash
cd python
pip install -e ".[dev]"
```

## Quick Start

```python
import colopresso

# Read PNG file
with open("input.png", "rb") as f:
    png_data = f.read()

# Convert to WebP
webp_data = colopresso.encode_webp(png_data)
with open("output.webp", "wb") as f:
    f.write(webp_data)

# Convert to AVIF
avif_data = colopresso.encode_avif(png_data)
with open("output.avif", "wb") as f:
    f.write(avif_data)

# Optimize PNG
optimized_png = colopresso.encode_pngx(png_data)
with open("output.png", "wb") as f:
    f.write(optimized_png)
```

---

## API Reference

### Encoding Functions

#### `encode_webp(png_data, config=None) -> bytes`

Encode PNG data to WebP format.

**Parameters:**
- `png_data` (bytes): Raw PNG file data
- `config` (Config, optional): Encoding configuration. Uses defaults if not provided.

**Returns:**
- bytes: WebP encoded data

**Raises:**
- `ColopressoError`: If encoding fails

**Example:**
```python
import colopresso

with open("input.png", "rb") as f:
    png_data = f.read()

# Default settings
webp_data = colopresso.encode_webp(png_data)

# Custom quality
config = colopresso.Config(webp_quality=90.0)
webp_data = colopresso.encode_webp(png_data, config)
```

---

#### `encode_avif(png_data, config=None) -> bytes`

Encode PNG data to AVIF format.

**Parameters:**
- `png_data` (bytes): Raw PNG file data
- `config` (Config, optional): Encoding configuration. Uses defaults if not provided.

**Returns:**
- bytes: AVIF encoded data

**Raises:**
- `ColopressoError`: If encoding fails (including when output is larger than input)

**Example:**
```python
import colopresso

with open("input.png", "rb") as f:
    png_data = f.read()

config = colopresso.Config(avif_quality=70.0, avif_speed=4)
avif_data = colopresso.encode_avif(png_data, config)
```

---

#### `encode_pngx(png_data, config=None) -> bytes`

Optimize PNG data using the PNGX encoder. Supports both lossless and lossy compression.

**Parameters:**
- `png_data` (bytes): Raw PNG file data
- `config` (Config, optional): Encoding configuration. Uses defaults if not provided.

**Returns:**
- bytes: Optimized PNG data

**Raises:**
- `ColopressoError`: If encoding fails

**Example:**
```python
import colopresso

with open("input.png", "rb") as f:
    png_data = f.read()

# 256-color palette optimization
config = colopresso.Config(
    pngx_lossy_enable=True,
    pngx_lossy_type=colopresso.PngxLossyType.PALETTE256,
    pngx_lossy_max_colors=256
)
optimized = colopresso.encode_pngx(png_data, config)
```

---

### Config Class

The `Config` dataclass holds all encoder settings.

```python
@dataclass
class Config:
    # WebP settings
    webp_quality: float = 80.0
    webp_lossless: bool = False
    webp_method: int = 6
    
    # AVIF settings
    avif_quality: float = 50.0
    avif_alpha_quality: int = 100
    avif_lossless: bool = False
    avif_speed: int = 6
    avif_threads: int = 1
    
    # PNGX settings
    pngx_level: int = 5
    pngx_strip_safe: bool = True
    pngx_optimize_alpha: bool = True
    pngx_lossy_enable: bool = True
    pngx_lossy_type: PngxLossyType = PngxLossyType.PALETTE256
    pngx_lossy_max_colors: int = 256
    pngx_lossy_quality_min: int = 80
    pngx_lossy_quality_max: int = 95
    pngx_lossy_speed: int = 3
    pngx_lossy_dither_level: float = 0.6
    pngx_threads: int = 1
```

---

### PngxLossyType Enum

Specifies the type of lossy compression for PNGX.

```python
class PngxLossyType(IntEnum):
    PALETTE256 = 0          # 256-color indexed palette
    LIMITED_RGBA4444 = 1    # RGBA4444 limited (16-bit color)
    REDUCED_RGBA32 = 2      # Reduced color RGBA32
```

| Value | Description | Use Case |
|---|---|---|
| `PALETTE256` | Convert to 256-color indexed palette | Icons, illustrations with few colors |
| `LIMITED_RGBA4444` | Limit each RGBA channel to 4 bits | Game textures, UI assets |
| `REDUCED_RGBA32` | Reduce colors while keeping RGBA32 | Photo-like images |

---

### Configuration Parameters

#### WebP Settings

| Parameter | Type | Default | Description |
|---|---|---|---|
| `webp_quality` | float | 80.0 | Quality (0.0-100.0). Higher = better quality, larger size |
| `webp_lossless` | bool | False | True: lossless compression, False: lossy compression |
| `webp_method` | int | 6 | Compression method (0-6). Higher = better compression, slower |

**Advanced WebP Parameters:**

| Parameter | Type | Default | Description |
|---|---|---|---|
| `webp_target_size` | int | 0 | Target file size in bytes. 0 = disabled |
| `webp_target_psnr` | float | 0.0 | Target PSNR in dB. 0 = disabled |
| `webp_segments` | int | 4 | Number of segments (1-4) |
| `webp_sns_strength` | int | 50 | Spatial noise shaping strength (0-100) |
| `webp_filter_strength` | int | 60 | Deblocking filter strength (0-100) |
| `webp_filter_sharpness` | int | 0 | Filter sharpness (0-7). 0 = sharpest |
| `webp_filter_type` | int | 1 | Filter type. 0 = simple, 1 = strong |
| `webp_autofilter` | bool | False | Auto-adjust filter strength |
| `webp_alpha_compression` | bool | True | Compress alpha channel |
| `webp_alpha_filtering` | int | 1 | Alpha filtering (0-2) |
| `webp_alpha_quality` | int | 100 | Alpha channel quality (0-100) |
| `webp_pass` | int | 1 | Number of encoding passes (1-10) |
| `webp_preprocessing` | int | 0 | Preprocessing. 0=none, 1=segment-smooth, 2=pseudo-random dither |
| `webp_partitions` | int | 0 | Number of partitions (0-3) |
| `webp_partition_limit` | int | 0 | Partition size limit (0-100) |
| `webp_emulate_jpeg_size` | bool | False | Emulate JPEG output size |
| `webp_thread_level` | int | 0 | Thread level (0-1) |
| `webp_low_memory` | bool | False | Low memory mode |
| `webp_near_lossless` | int | 100 | Near-lossless preprocessing (0-100). 100 = off |
| `webp_exact` | bool | False | Preserve RGB values in transparent areas |
| `webp_use_delta_palette` | bool | False | Use delta palette (experimental) |
| `webp_use_sharp_yuv` | bool | False | Use sharp RGB to YUV conversion |

---

#### AVIF Settings

| Parameter | Type | Default | Description |
|---|---|---|---|
| `avif_quality` | float | 50.0 | Quality (0.0-100.0). Higher = better quality, larger size |
| `avif_alpha_quality` | int | 100 | Alpha channel quality (0-100) |
| `avif_lossless` | bool | False | True: lossless compression, False: lossy compression |
| `avif_speed` | int | 6 | Encoding speed (0-10). 0 = best quality/slowest, 10 = lowest quality/fastest |
| `avif_threads` | int | 1 | Number of threads for encoding |

**Recommended Settings:**

| Use Case | quality | speed | Description |
|---|---|---|---|
| High Quality | 80-90 | 4 | High quality web images |
| Balanced | 50-60 | 6 | General purpose |
| Fast | 40-50 | 8-10 | Batch processing, previews |

---

#### PNGX Settings

##### Basic Settings

| Parameter | Type | Default | Description |
|---|---|---|---|
| `pngx_level` | int | 5 | Compression level (1-6). Higher = better compression |
| `pngx_strip_safe` | bool | True | Remove safely removable metadata |
| `pngx_optimize_alpha` | bool | True | Optimize color info in transparent pixels |
| `pngx_threads` | int | 1 | Number of threads for processing |

##### Lossy Compression Settings

| Parameter | Type | Default | Description |
|---|---|---|---|
| `pngx_lossy_enable` | bool | True | Enable lossy compression |
| `pngx_lossy_type` | PngxLossyType | PALETTE256 | Type of lossy compression |
| `pngx_lossy_max_colors` | int | 256 | Maximum colors for palette mode (2-256) |
| `pngx_lossy_quality_min` | int | 80 | Minimum quality (0-100) |
| `pngx_lossy_quality_max` | int | 95 | Maximum quality (0-100) |
| `pngx_lossy_speed` | int | 3 | Quantization speed (1-10). Higher = faster, lower quality |
| `pngx_lossy_dither_level` | float | 0.6 | Dithering strength (0.0-1.0) |

##### PALETTE256 Mode Settings

| Parameter | Type | Default | Description |
|---|---|---|---|
| `pngx_lossy_reduced_colors` | int | 0 | Target color count after reduction. 0 = auto |
| `pngx_saliency_map_enable` | bool | False | Adaptive quantization using saliency map |
| `pngx_chroma_anchor_enable` | bool | False | Preserve chroma anchor points |
| `pngx_adaptive_dither_enable` | bool | False | Adaptive dithering |
| `pngx_gradient_boost_enable` | bool | False | Gradient enhancement |
| `pngx_chroma_weight_enable` | bool | False | Apply chroma weighting |
| `pngx_postprocess_smooth_enable` | bool | False | Post-processing smoothing |
| `pngx_postprocess_smooth_importance_cutoff` | float | 0.0 | Smoothing importance cutoff |
| `pngx_palette256_gradient_profile_enable` | bool | False | Enable gradient profile |
| `pngx_palette256_gradient_dither_floor` | float | 0.0 | Gradient dither floor |
| `pngx_palette256_alpha_bleed_enable` | bool | False | Enable alpha bleed |
| `pngx_palette256_alpha_bleed_max_distance` | int | 0 | Maximum bleed distance in pixels |
| `pngx_palette256_alpha_bleed_opaque_threshold` | int | 0 | Opaque threshold |
| `pngx_palette256_alpha_bleed_soft_limit` | int | 0 | Soft bleed limit |

##### LIMITED_RGBA4444 Mode Settings

| Parameter | Type | Default | Description |
|---|---|---|---|
| `pngx_lossy_reduced_bits_rgb` | int | 4 | RGB channel bit depth (1-8) |
| `pngx_lossy_reduced_alpha_bits` | int | 4 | Alpha channel bit depth (1-8) |

---

### Utility Functions

#### Version Information

```python
def get_version() -> int
```
Get the colopresso version number.

```python
def get_libwebp_version() -> int
```
Get the libwebp version number (e.g., 67072 = 1.4.0).

```python
def get_libpng_version() -> int
```
Get the libpng version number.

```python
def get_libavif_version() -> int
```
Get the libavif version number.

```python
def get_pngx_oxipng_version() -> int
```
Get the oxipng version number.

```python
def get_pngx_libimagequant_version() -> int
```
Get the libimagequant version number.

#### Build Information

```python
def get_buildtime() -> int
```
Get the build timestamp (Unix time).

```python
def get_compiler_version_string() -> str
```
Get the C compiler version string.

```python
def get_rust_version_string() -> str
```
Get the Rust compiler version string.

#### Thread Information

```python
def is_threads_enabled() -> bool
```
Check if multithreading is enabled.

```python
def get_default_thread_count() -> int
```
Get the default number of threads.

```python
def get_max_thread_count() -> int
```
Get the maximum available thread count.

---

### Exception Classes

#### ColopressoError

```python
class ColopressoError(Exception):
    code: int       # Error code
    message: str    # Error message
```

**Error Codes:**

| Code | Name | Description |
|---|---|---|
| 0 | OK | Success |
| 1 | File not found | File not found |
| 2 | Invalid PNG | Invalid PNG data |
| 3 | Invalid format | Invalid format |
| 4 | Out of memory | Memory allocation failed |
| 5 | Encode failed | Encoding failed |
| 6 | Decode failed | Decoding failed |
| 7 | IO error | Input/output error |
| 8 | Invalid parameter | Invalid parameter |
| 9 | Output not smaller | Output is not smaller than input |

**Example:**
```python
import colopresso

try:
    result = colopresso.encode_avif(png_data)
except colopresso.ColopressoError as e:
    if e.code == 9:
        print("AVIF compression was not effective. Using original PNG.")
    else:
        print(f"Error: {e.message}")
```

---

## Examples

### WebP Conversion with Quality Settings

```python
import colopresso

with open("photo.png", "rb") as f:
    png_data = f.read()

# High quality WebP
config = colopresso.Config(
    webp_quality=95.0,
    webp_method=6,
    webp_use_sharp_yuv=True
)
high_quality = colopresso.encode_webp(png_data, config)

# High compression WebP
config = colopresso.Config(
    webp_quality=60.0,
    webp_method=6
)
compressed = colopresso.encode_webp(png_data, config)

# Lossless WebP
config = colopresso.Config(webp_lossless=True)
lossless = colopresso.encode_webp(png_data, config)
```

### AVIF with Multithreading

```python
import colopresso

with open("large_image.png", "rb") as f:
    png_data = f.read()

# Multithreaded high-speed encoding
config = colopresso.Config(
    avif_quality=70.0,
    avif_speed=6,
    avif_threads=4
)
avif_data = colopresso.encode_avif(png_data, config)
```

### PNG Optimization (256-Color Palette)

```python
import colopresso

with open("icon.png", "rb") as f:
    png_data = f.read()

config = colopresso.Config(
    pngx_lossy_enable=True,
    pngx_lossy_type=colopresso.PngxLossyType.PALETTE256,
    pngx_lossy_max_colors=256,
    pngx_lossy_quality_min=85,
    pngx_lossy_quality_max=100,
    pngx_lossy_dither_level=0.8
)
optimized = colopresso.encode_pngx(png_data, config)

print(f"Original: {len(png_data)} bytes")
print(f"Optimized: {len(optimized)} bytes")
print(f"Reduction: {(1 - len(optimized)/len(png_data))*100:.1f}%")
```

### PNG Lossless Optimization

```python
import colopresso

with open("screenshot.png", "rb") as f:
    png_data = f.read()

config = colopresso.Config(
    pngx_lossy_enable=False,  # Disable lossy compression
    pngx_level=6,             # Maximum compression level
    pngx_strip_safe=True,     # Remove metadata
    pngx_optimize_alpha=True  # Optimize alpha
)
optimized = colopresso.encode_pngx(png_data, config)
```

### Game Texture PNG (RGBA4444)

```python
import colopresso

with open("texture.png", "rb") as f:
    png_data = f.read()

# RGBA4444 format (16-bit color)
config = colopresso.Config(
    pngx_lossy_enable=True,
    pngx_lossy_type=colopresso.PngxLossyType.LIMITED_RGBA4444,
    pngx_lossy_reduced_bits_rgb=4,
    pngx_lossy_reduced_alpha_bits=4
)
optimized = colopresso.encode_pngx(png_data, config)
```

### Batch Processing

```python
import colopresso
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor

def convert_to_webp(png_path: Path) -> tuple[Path, int, int]:
    """Convert PNG to WebP and return size info"""
    with open(png_path, "rb") as f:
        png_data = f.read()
    
    config = colopresso.Config(webp_quality=80.0)
    webp_data = colopresso.encode_webp(png_data, config)
    
    webp_path = png_path.with_suffix(".webp")
    with open(webp_path, "wb") as f:
        f.write(webp_data)
    
    return webp_path, len(png_data), len(webp_data)

# Parallel conversion
png_files = list(Path("images").glob("*.png"))
with ThreadPoolExecutor(max_workers=4) as executor:
    results = list(executor.map(convert_to_webp, png_files))

for path, original, converted in results:
    print(f"{path.name}: {original} -> {converted} ({(1-converted/original)*100:.1f}% reduction)")
```

### Error Handling

```python
import colopresso

def safe_encode(png_data: bytes, format: str = "webp") -> bytes | None:
    """Encode with error handling"""
    config = colopresso.Config()
    
    try:
        if format == "webp":
            return colopresso.encode_webp(png_data, config)
        elif format == "avif":
            return colopresso.encode_avif(png_data, config)
        elif format == "pngx":
            return colopresso.encode_pngx(png_data, config)
        else:
            raise ValueError(f"Unknown format: {format}")
            
    except colopresso.ColopressoError as e:
        if e.code == 2:
            print("Error: Input is not a valid PNG")
        elif e.code == 9:
            print(f"{format.upper()} compression was not effective")
            return png_data  # Return original data
        else:
            print(f"Encoding error: {e.message}")
        return None

# Usage
with open("input.png", "rb") as f:
    png_data = f.read()

result = safe_encode(png_data, "avif")
if result:
    with open("output.avif", "wb") as f:
        f.write(result)
```

---

## Building Wheels

```bash
cd python
pip install build
python -m build --wheel
```

The wheel will be created in `python/dist/`.

---

## License

colopresso is licensed under GPL-3.0-or-later.

Copyright (C) 2026 COLOPL, Inc.
