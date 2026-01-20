# Colopresso Python バインディング

colopresso 画像圧縮ライブラリの Python バインディングです。PNG 画像を WebP、AVIF、または最適化 PNG (PNGX) フォーマットにエンコードできます。

## 目次

- [動作要件](#動作要件)
- [インストール](#インストール)
- [クイックスタート](#クイックスタート)
- [API リファレンス](#api-リファレンス)
  - [エンコード関数](#エンコード関数)
  - [Config クラス](#config-クラス)
  - [PngxLossyType 列挙型](#pngxlossytype-列挙型)
  - [設定パラメータ](#設定パラメータ)
  - [ユーティリティ関数](#ユーティリティ関数)
  - [例外クラス](#例外クラス)
- [使用例](#使用例)
- [Wheel のビルド](#wheel-のビルド)

## 動作要件

> [!IMPORTANT]
> **x86_64 (amd64) 環境では AVX2 命令セットのサポートが必須です。**
> 2013年以降の Intel Haswell 以降、または AMD Excavator 以降のプロセッサが必要です。

| プラットフォーム | アーキテクチャ | 必要条件 |
|-----------------|---------------|---------|
| Windows | x64 | AVX2 サポート |
| Windows | ARM64 | - |
| macOS | x86_64 | AVX2 サポート |
| macOS | arm64 (Apple Silicon) | - |
| Linux (glibc) | x86_64 | AVX2 サポート |
| Linux (glibc) | aarch64 | - |
| Linux (musl) | x86_64 | AVX2 サポート |
| Linux (musl) | aarch64 | - |

## インストール

### PyPI から

```bash
pip install "colopresso"
```

### ソースから (CMake、Rust、C コンパイラが必要)

```bash
cd python
pip install .
```

### 開発用インストール

```bash
cd python
pip install -e ".[dev]"
```

## クイックスタート

```python
import colopresso

# PNG ファイルを読み込む
with open("input.png", "rb") as f:
    png_data = f.read()

# WebP に変換
webp_data = colopresso.encode_webp(png_data)
with open("output.webp", "wb") as f:
    f.write(webp_data)

# AVIF に変換
avif_data = colopresso.encode_avif(png_data)
with open("output.avif", "wb") as f:
    f.write(avif_data)

# PNG を最適化
optimized_png = colopresso.encode_pngx(png_data)
with open("output.png", "wb") as f:
    f.write(optimized_png)
```

---

## API リファレンス

### エンコード関数

#### `encode_webp(png_data, config=None) -> bytes`

PNG データを WebP フォーマットにエンコードします。

**パラメータ:**
- `png_data` (bytes): 生の PNG ファイルデータ
- `config` (Config, optional): エンコード設定。指定しない場合はデフォルト値を使用。

**戻り値:**
- bytes: WebP エンコードデータ

**例外:**
- `ColopressoError`: エンコードに失敗した場合

**例:**
```python
import colopresso

with open("input.png", "rb") as f:
    png_data = f.read()

# デフォルト設定
webp_data = colopresso.encode_webp(png_data)

# カスタム品質
config = colopresso.Config(webp_quality=90.0)
webp_data = colopresso.encode_webp(png_data, config)
```

---

#### `encode_avif(png_data, config=None) -> bytes`

PNG データを AVIF フォーマットにエンコードします。

**パラメータ:**
- `png_data` (bytes): 生の PNG ファイルデータ
- `config` (Config, optional): エンコード設定。指定しない場合はデフォルト値を使用。

**戻り値:**
- bytes: AVIF エンコードデータ

**例外:**
- `ColopressoError`: エンコードに失敗した場合 (出力が入力より大きい場合を含む)

**例:**
```python
import colopresso

with open("input.png", "rb") as f:
    png_data = f.read()

config = colopresso.Config(avif_quality=70.0, avif_speed=4)
avif_data = colopresso.encode_avif(png_data, config)
```

---

#### `encode_pngx(png_data, config=None) -> bytes`

PNGX エンコーダを使用して PNG データを最適化します。ロスレスとロッシー圧縮の両方をサポートしています。

**パラメータ:**
- `png_data` (bytes): 生の PNG ファイルデータ
- `config` (Config, optional): エンコード設定。指定しない場合はデフォルト値を使用。

**戻り値:**
- bytes: 最適化された PNG データ

**例外:**
- `ColopressoError`: エンコードに失敗した場合

**例:**
```python
import colopresso

with open("input.png", "rb") as f:
    png_data = f.read()

# 256色パレット最適化
config = colopresso.Config(
    pngx_lossy_enable=True,
    pngx_lossy_type=colopresso.PngxLossyType.PALETTE256,
    pngx_lossy_max_colors=256
)
optimized = colopresso.encode_pngx(png_data, config)
```

---

### Config クラス

`Config` データクラスはすべてのエンコーダ設定を保持します。

```python
@dataclass
class Config:
    # WebP 設定
    webp_quality: float = 80.0
    webp_lossless: bool = False
    webp_method: int = 6
    
    # AVIF 設定
    avif_quality: float = 50.0
    avif_alpha_quality: int = 100
    avif_lossless: bool = False
    avif_speed: int = 6
    avif_threads: int = 1
    
    # PNGX 設定
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

### PngxLossyType 列挙型

PNGX のロッシー圧縮タイプを指定します。

```python
class PngxLossyType(IntEnum):
    PALETTE256 = 0          # 256色インデックスパレット
    LIMITED_RGBA4444 = 1    # RGBA4444 制限 (16ビットカラー)
    REDUCED_RGBA32 = 2      # 減色 RGBA32
```

| 値 | 説明 | 用途 |
|---|---|---|
| `PALETTE256` | 256色インデックスパレットに変換 | アイコン、色数の少ないイラスト |
| `LIMITED_RGBA4444` | 各 RGBA チャンネルを 4 ビットに制限 | ゲームテクスチャ、UI アセット |
| `REDUCED_RGBA32` | RGBA32 を保持しながら色数を削減 | 写真のような画像 |

---

### 設定パラメータ

#### WebP 設定

| パラメータ | 型 | デフォルト | 説明 |
|---|---|---|---|
| `webp_quality` | float | 80.0 | 品質 (0.0-100.0)。高いほど高品質、サイズも大きい |
| `webp_lossless` | bool | False | True: ロスレス圧縮、False: ロッシー圧縮 |
| `webp_method` | int | 6 | 圧縮方式 (0-6)。高いほど高圧縮、低速 |

**WebP 詳細パラメータ:**

| パラメータ | 型 | デフォルト | 説明 |
|---|---|---|---|
| `webp_target_size` | int | 0 | 目標ファイルサイズ (バイト)。0 = 無効 |
| `webp_target_psnr` | float | 0.0 | 目標 PSNR (dB)。0 = 無効 |
| `webp_segments` | int | 4 | セグメント数 (1-4) |
| `webp_sns_strength` | int | 50 | 空間ノイズ整形強度 (0-100) |
| `webp_filter_strength` | int | 60 | デブロッキングフィルタ強度 (0-100) |
| `webp_filter_sharpness` | int | 0 | フィルタシャープネス (0-7)。0 = 最もシャープ |
| `webp_filter_type` | int | 1 | フィルタタイプ。0 = シンプル、1 = 強力 |
| `webp_autofilter` | bool | False | フィルタ強度を自動調整 |
| `webp_alpha_compression` | bool | True | アルファチャンネルを圧縮 |
| `webp_alpha_filtering` | int | 1 | アルファフィルタリング (0-2) |
| `webp_alpha_quality` | int | 100 | アルファチャンネル品質 (0-100) |
| `webp_pass` | int | 1 | エンコードパス数 (1-10) |
| `webp_preprocessing` | int | 0 | 前処理。0=なし、1=セグメントスムーズ、2=擬似ランダムディザ |
| `webp_partitions` | int | 0 | パーティション数 (0-3) |
| `webp_partition_limit` | int | 0 | パーティションサイズ制限 (0-100) |
| `webp_emulate_jpeg_size` | bool | False | JPEG 出力サイズをエミュレート |
| `webp_thread_level` | int | 0 | スレッドレベル (0-1) |
| `webp_low_memory` | bool | False | 低メモリモード |
| `webp_near_lossless` | int | 100 | ニアロスレス前処理 (0-100)。100 = オフ |
| `webp_exact` | bool | False | 透明部分の RGB 値を保持 |
| `webp_use_delta_palette` | bool | False | デルタパレットを使用 (実験的) |
| `webp_use_sharp_yuv` | bool | False | シャープな RGB から YUV への変換を使用 |

---

#### AVIF 設定

| パラメータ | 型 | デフォルト | 説明 |
|---|---|---|---|
| `avif_quality` | float | 50.0 | 品質 (0.0-100.0)。高いほど高品質、サイズも大きい |
| `avif_alpha_quality` | int | 100 | アルファチャンネル品質 (0-100) |
| `avif_lossless` | bool | False | True: ロスレス圧縮、False: ロッシー圧縮 |
| `avif_speed` | int | 6 | エンコード速度 (0-10)。0 = 最高品質/最低速、10 = 最低品質/最高速 |
| `avif_threads` | int | 1 | エンコードに使用するスレッド数 |

**推奨設定:**

| 用途 | quality | speed | 説明 |
|---|---|---|---|
| 高品質 | 80-90 | 4 | 高品質な Web 画像 |
| バランス | 50-60 | 6 | 汎用 |
| 高速 | 40-50 | 8-10 | バッチ処理、プレビュー |

---

#### PNGX 設定

##### 基本設定

| パラメータ | 型 | デフォルト | 説明 |
|---|---|---|---|
| `pngx_level` | int | 5 | 圧縮レベル (1-6)。高いほど高圧縮 |
| `pngx_strip_safe` | bool | True | 安全に削除可能なメタデータを削除 |
| `pngx_optimize_alpha` | bool | True | 透明ピクセルのカラー情報を最適化 |
| `pngx_threads` | int | 1 | 処理に使用するスレッド数 |

##### ロッシー圧縮設定

| パラメータ | 型 | デフォルト | 説明 |
|---|---|---|---|
| `pngx_lossy_enable` | bool | True | ロッシー圧縮を有効化 |
| `pngx_lossy_type` | PngxLossyType | PALETTE256 | ロッシー圧縮のタイプ |
| `pngx_lossy_max_colors` | int | 256 | パレットモードの最大色数 (2-256) |
| `pngx_lossy_quality_min` | int | 80 | 最小品質 (0-100) |
| `pngx_lossy_quality_max` | int | 95 | 最大品質 (0-100) |
| `pngx_lossy_speed` | int | 3 | 量子化速度 (1-10)。高いほど高速、低品質 |
| `pngx_lossy_dither_level` | float | 0.6 | ディザリング強度 (0.0-1.0) |

##### PALETTE256 モード設定

| パラメータ | 型 | デフォルト | 説明 |
|---|---|---|---|
| `pngx_lossy_reduced_colors` | int | 0 | 削減後の目標色数。0 = 自動 |
| `pngx_saliency_map_enable` | bool | False | サリエンシーマップを使用した適応量子化 |
| `pngx_chroma_anchor_enable` | bool | False | クロマアンカーポイントを保持 |
| `pngx_adaptive_dither_enable` | bool | False | 適応ディザリング |
| `pngx_gradient_boost_enable` | bool | False | グラデーション強調 |
| `pngx_chroma_weight_enable` | bool | False | クロマ重み付けを適用 |
| `pngx_postprocess_smooth_enable` | bool | False | 後処理スムージング |
| `pngx_postprocess_smooth_importance_cutoff` | float | 0.0 | スムージング重要度カットオフ |
| `pngx_palette256_gradient_profile_enable` | bool | False | グラデーションプロファイルを有効化 |
| `pngx_palette256_gradient_dither_floor` | float | 0.0 | グラデーションディザフロア |
| `pngx_palette256_alpha_bleed_enable` | bool | False | アルファブリードを有効化 |
| `pngx_palette256_alpha_bleed_max_distance` | int | 0 | 最大ブリード距離 (ピクセル) |
| `pngx_palette256_alpha_bleed_opaque_threshold` | int | 0 | 不透明しきい値 |
| `pngx_palette256_alpha_bleed_soft_limit` | int | 0 | ソフトブリード制限 |

##### LIMITED_RGBA4444 モード設定

| パラメータ | 型 | デフォルト | 説明 |
|---|---|---|---|
| `pngx_lossy_reduced_bits_rgb` | int | 4 | RGB チャンネルのビット深度 (1-8) |
| `pngx_lossy_reduced_alpha_bits` | int | 4 | アルファチャンネルのビット深度 (1-8) |

---

### ユーティリティ関数

#### バージョン情報

```python
def get_version() -> int
```
colopresso のバージョン番号を取得します。

```python
def get_libwebp_version() -> int
```
libwebp のバージョン番号を取得します (例: 67072 = 1.4.0)。

```python
def get_libpng_version() -> int
```
libpng のバージョン番号を取得します。

```python
def get_libavif_version() -> int
```
libavif のバージョン番号を取得します。

```python
def get_pngx_oxipng_version() -> int
```
oxipng のバージョン番号を取得します。

```python
def get_pngx_libimagequant_version() -> int
```
libimagequant のバージョン番号を取得します。

#### ビルド情報

```python
def get_buildtime() -> int
```
ビルドタイムスタンプ (Unix 時間) を取得します。

```python
def get_compiler_version_string() -> str
```
C コンパイラのバージョン文字列を取得します。

```python
def get_rust_version_string() -> str
```
Rust コンパイラのバージョン文字列を取得します。

#### スレッド情報

```python
def is_threads_enabled() -> bool
```
マルチスレッドが有効かどうかを確認します。

```python
def get_default_thread_count() -> int
```
デフォルトのスレッド数を取得します。

```python
def get_max_thread_count() -> int
```
利用可能な最大スレッド数を取得します。

---

### 例外クラス

#### ColopressoError

```python
class ColopressoError(Exception):
    code: int       # エラーコード
    message: str    # エラーメッセージ
```

**エラーコード:**

| コード | 名前 | 説明 |
|---|---|---|
| 0 | OK | 成功 |
| 1 | File not found | ファイルが見つからない |
| 2 | Invalid PNG | 無効な PNG データ |
| 3 | Invalid format | 無効なフォーマット |
| 4 | Out of memory | メモリ割り当て失敗 |
| 5 | Encode failed | エンコード失敗 |
| 6 | Decode failed | デコード失敗 |
| 7 | IO error | 入出力エラー |
| 8 | Invalid parameter | 無効なパラメータ |
| 9 | Output not smaller | 出力が入力より小さくない |

**例:**
```python
import colopresso

try:
    result = colopresso.encode_avif(png_data)
except colopresso.ColopressoError as e:
    if e.code == 9:
        print("AVIF 圧縮が効果的ではありませんでした。元の PNG を使用します。")
    else:
        print(f"エラー: {e.message}")
```

---

## 使用例

### 品質設定付き WebP 変換

```python
import colopresso

with open("photo.png", "rb") as f:
    png_data = f.read()

# 高品質 WebP
config = colopresso.Config(
    webp_quality=95.0,
    webp_method=6,
    webp_use_sharp_yuv=True
)
high_quality = colopresso.encode_webp(png_data, config)

# 高圧縮 WebP
config = colopresso.Config(
    webp_quality=60.0,
    webp_method=6
)
compressed = colopresso.encode_webp(png_data, config)

# ロスレス WebP
config = colopresso.Config(webp_lossless=True)
lossless = colopresso.encode_webp(png_data, config)
```

### マルチスレッド対応 AVIF

```python
import colopresso

with open("large_image.png", "rb") as f:
    png_data = f.read()

# マルチスレッド高速エンコード
config = colopresso.Config(
    avif_quality=70.0,
    avif_speed=6,
    avif_threads=4
)
avif_data = colopresso.encode_avif(png_data, config)
```

### PNG 最適化 (256色パレット)

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

print(f"元サイズ: {len(png_data)} バイト")
print(f"最適化後: {len(optimized)} バイト")
print(f"削減率: {(1 - len(optimized)/len(png_data))*100:.1f}%")
```

### PNG ロスレス最適化

```python
import colopresso

with open("screenshot.png", "rb") as f:
    png_data = f.read()

config = colopresso.Config(
    pngx_lossy_enable=False,  # ロッシー圧縮を無効化
    pngx_level=6,             # 最大圧縮レベル
    pngx_strip_safe=True,     # メタデータを削除
    pngx_optimize_alpha=True  # アルファを最適化
)
optimized = colopresso.encode_pngx(png_data, config)
```

### ゲームテクスチャ PNG (RGBA4444)

```python
import colopresso

with open("texture.png", "rb") as f:
    png_data = f.read()

# RGBA4444 フォーマット (16ビットカラー)
config = colopresso.Config(
    pngx_lossy_enable=True,
    pngx_lossy_type=colopresso.PngxLossyType.LIMITED_RGBA4444,
    pngx_lossy_reduced_bits_rgb=4,
    pngx_lossy_reduced_alpha_bits=4
)
optimized = colopresso.encode_pngx(png_data, config)
```

### バッチ処理

```python
import colopresso
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor

def convert_to_webp(png_path: Path) -> tuple[Path, int, int]:
    """PNG を WebP に変換してサイズ情報を返す"""
    with open(png_path, "rb") as f:
        png_data = f.read()
    
    config = colopresso.Config(webp_quality=80.0)
    webp_data = colopresso.encode_webp(png_data, config)
    
    webp_path = png_path.with_suffix(".webp")
    with open(webp_path, "wb") as f:
        f.write(webp_data)
    
    return webp_path, len(png_data), len(webp_data)

# 並列変換
png_files = list(Path("images").glob("*.png"))
with ThreadPoolExecutor(max_workers=4) as executor:
    results = list(executor.map(convert_to_webp, png_files))

for path, original, converted in results:
    print(f"{path.name}: {original} -> {converted} ({(1-converted/original)*100:.1f}% 削減)")
```

### エラーハンドリング

```python
import colopresso

def safe_encode(png_data: bytes, format: str = "webp") -> bytes | None:
    """エラーハンドリング付きエンコード"""
    config = colopresso.Config()
    
    try:
        if format == "webp":
            return colopresso.encode_webp(png_data, config)
        elif format == "avif":
            return colopresso.encode_avif(png_data, config)
        elif format == "pngx":
            return colopresso.encode_pngx(png_data, config)
        else:
            raise ValueError(f"不明なフォーマット: {format}")
            
    except colopresso.ColopressoError as e:
        if e.code == 2:
            print("エラー: 入力が有効な PNG ではありません")
        elif e.code == 9:
            print(f"{format.upper()} 圧縮が効果的ではありませんでした")
            return png_data  # 元のデータを返す
        else:
            print(f"エンコードエラー: {e.message}")
        return None

# 使用例
with open("input.png", "rb") as f:
    png_data = f.read()

result = safe_encode(png_data, "avif")
if result:
    with open("output.avif", "wb") as f:
        f.write(result)
```

---

## Wheel のビルド

```bash
cd python
pip install build
python -m build --wheel
```

Wheel は `python/dist/` に作成されます。

---

## ライセンス

colopresso は GPL-3.0-or-later でライセンスされています。

Copyright (C) 2026 COLOPL, Inc.
