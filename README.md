# libcolopresso / colopresso

PNG to Advanced Format converter and optimizer library and application.

[日本語版](./README_ja.md)

<p align="center">
  <img src="resources/icon.png" alt="colopresso icon" />
</p>
<p align="center">
  <img src="resources/screenshot.png" alt="colopresso screenshot" width="50%" />
</p>

## Table of Contents

- [Overview](#overview)
- [Variants](#variants)
- [Development Environment](#development-environment)
- [Build Options](#build-options)
- [Build (Linux)](#build-linux)
- [Build (macOS)](#build-macos)
- [Build (Node.js)](#build-nodejs)
- [Build (Chrome Extension)](#build-chrome-extension)
- [Build (Electron)](#build-electron)
- [License](#license)
- [Notices](#notices)

## Variants

### CLI

- Runs as a console application on each OS
- Fully utilizes OS-level multithreading and CPU extensions for performance
- Available on Windows, Linux, and macOS

### Chrome Extension

- Runs as a Google Chrome extension
- Converted files are downloaded directly by Chrome
- Supports dropping folders or multiple PNG files at once and downloading the converted files as a ZIP archive (optional setting)

> [!NOTE]
> The official Chrome Extension is not published on the Chrome Web Store at the moment, and there are currently no plans to release it.

### Electron

- Runs as a desktop application
- Converts and saves every PNG file contained in the selected or dropped folder
    - You can optionally delete the original PNG files after conversion

### Node.js (with WebAssembly)

- If Node.js 18 or later is available, you can run the Emscripten-built CLI (`colopresso.js`) as-is
- When both `COLOPRESSO_NODE_BUILD=ON` and `COLOPRESSO_USE_CLI=ON` are set, `./build/cli/colopresso.js` and `colopresso.wasm` are generated, enabling commands such as `node ./build/cli/colopresso.js --help`

### Shared Features

- Supported output formats
    - WebP (lossy / lossless)
    - AVIF (lossy / lossless)
    - PNG (256-palette, Reduced RGBA32, Limited RGBA4444, lossless)
        - Lossless compression (metadata removal)
        - 256-color quantization (with protected colors)
        - Reduced RGBA32 quantization (auto or manual color targets, preserves 8-bit RGBA output)
            - Control RGB / alpha grid bit depths (1-8, default 4) via CLI (`--reduce-bits-rgb`, `--reduce-alpha`) or the GUI advanced panel, with importance-aware dithering
            - Importance-weighted clustering with iterative refinement preserves PSNR while still delivering smaller RGBA PNGs
        - Limited RGBA4444 bit-depth quantization with error-diffusion dithering and auto-dither heuristics 
            - set `--dither -1` or leave GUI field blank to auto-estimate

- Profile feature
    - Fine-tune per-format parameters, then save, export, or import profiles

#### Format Guidance

- If the minimum supported device is iOS 16 or later, you can use AVIF, which generally delivers the best visual quality.
- If the minimum supported device is iOS 14 or later, use WebP.
- If the minimum supported device is earlier than iOS 14, use PNG.

> [!NOTE]
> Android 5.x and later always rely on Chromium via Google Chrome, so no additional considerations are required (all formats are supported).

## Development Environment

```bash
$ git clone --recursive "https://github.com/colopl/colopresso.git"
```

Open the cloned `libcolopresso` directory with Visual Studio Code and attach to the Dev Container using the `Dev Containers` extension.

> [!NOTE]
> When MemorySanitizer runs on arm64 with SIMD (NEON) enabled, `libpng` might be flagged for reading uninitialized memory. We currently disable SIMD (NEON) during MemorySanitizer runs.

> [!NOTE]
> On i386 / amd64, enabling assembly while using MemorySanitizer may trigger false positives in `libaom`. We currently disable assembler code when running MemorySanitizer.

> [!NOTE]
> With multithreading enabled, Valgrind / MemorySanitizer may report uninitialized memory access or leaked resources because of Rayon’s design. Suppression files are provided to mitigate this.

## Build Options

### Always Available

- `COLOPRESSO_USE_CLI`        ON/OFF [default: OFF] Enables building the CLI binary. Requires `COLOPRESSO_WITH_FILE_OPS=ON`.
- `COLOPRESSO_USE_UTILS`      ON/OFF [default: OFF] Builds code under `library/utils/`. Automatically disabled if `COLOPRESSO_WITH_FILE_OPS=OFF`.
- `COLOPRESSO_USE_TESTS`      ON/OFF [default: OFF] Builds code under `library/tests/`.
- `COLOPRESSO_WITH_FILE_OPS`  ON/OFF [default: ON] Enables file I/O APIs (`cpres_encode_*_file`, `cpres_png_decode_from_file`). Relies on standard file I/O (`fopen`, `fwrite`, etc.). Memory-based APIs (`cpres_encode_*_memory`) are always available. Enabling Chrome Extension or Electron builds forces this to `OFF`.

### GCC (GNU C Compiler) && `-DCMAKE_BUILD_TYPE=Debug`
- `COLOPRESSO_USE_VALGRIND`     ON/OFF Enables Valgrind integration if available.
- `COLOPRESSO_USE_COVERAGE`     ON/OFF Enables `gcov` coverage if available.

### Clang && `-DCMAKE_BUILD_TYPE=Debug`
- `COLOPRESSO_USE_ASAN`         ON/OFF Enables AddressSanitizer if available.
- `COLOPRESSO_USE_MSAN`         ON/OFF Enables MemorySanitizer if available.
- `COLOPRESSO_USE_UBSAN`        ON/OFF Enables UndefinedBehaviorSanitizer if available.

## Build (Linux)

1. Install VS Code and Docker (or compatible software), then open the repository directory
2. Attach using Dev Containers
3. `rm -rf "build" && cmake -B "build" -DCMAKE_BUILD_TYPE=Release -DCOLOPRESSO_USE_UTILS=ON -DCOLOPRESSO_USE_TESTS=ON -DCOLOPRESSO_USE_CLI=ON`
4. `cmake --build "build" --parallel`
5. `ctest --test-dir "build" --output-on-failure --parallel`
6. `./build/cli/colopresso` holds the CLI binary, `./build/utils` contains utility binaries, and `./build` contains `libcolopresso.a`

### Coverage Output

1. `rm -rf "build" && cmake -B "build" -DCMAKE_BUILD_TYPE=Debug -DCOLOPRESSO_USE_COVERAGE=ON -DCOLOPRESSO_USE_TESTS=ON`
2. `cmake --build "build" --parallel`
3. `cmake --build "build" --target coverage`
4. Coverage reports are generated under `./build/coverage/html/`

### Valgrind Check

1. `rm -rf "build" && cmake -B "build" -DCMAKE_BUILD_TYPE=Debug -DCOLOPRESSO_USE_VALGRIND=ON -DCOLOPRESSO_USE_TESTS=ON`
2. `cmake --build "build" --parallel`
3. `ctest --test-dir "build" --output-on-failure --parallel`

#### Notes (speed vs detail)

The Valgrind test suite includes several encoder end-to-end tests and can be extremely slow on CI.
To keep coverage while reducing runtime, the Valgrind runner is configurable via these CMake options:

- `COLOPRESSO_VALGRIND_TRACK_ORIGINS` (default: `OFF`)
    - When `ON`, adds Valgrind `--track-origins=yes`.
    - This is **very slow**, but it helps attribute where uninitialized values come from (useful when chasing `Memcheck:Value*` / `Memcheck:Cond` reports).

- `COLOPRESSO_VALGRIND_RAYON_NUM_THREADS` (default: `1`)
    - Sets `RAYON_NUM_THREADS` for Valgrind tests.
    - Limits oversubscription and reduces non-determinism and runtime when Rust/Rayon is involved.

- `COLOPRESSO_VALGRIND_LEAK_CHECK` (default: `full`)
    - Controls Valgrind `--leak-check` (`no|summary|full`).

- `COLOPRESSO_VALGRIND_SHOW_LEAK_KINDS` (default: `all`)
    - Controls Valgrind `--show-leak-kinds` (e.g. `definite,indirect,possible,reachable` or `all`).

Examples:

- Fast CI-like Valgrind run:
    - `rm -rf "build" && cmake -B "build" -DCMAKE_BUILD_TYPE=Debug -DCOLOPRESSO_USE_VALGRIND=ON -DCOLOPRESSO_USE_TESTS=ON -DCOLOPRESSO_VALGRIND_TRACK_ORIGINS=OFF -DCOLOPRESSO_VALGRIND_RAYON_NUM_THREADS=1`

- Deep investigation (uninitialized origin tracking):
    - `rm -rf "build" && cmake -B "build" -DCMAKE_BUILD_TYPE=Debug -DCOLOPRESSO_USE_VALGRIND=ON -DCOLOPRESSO_USE_TESTS=ON -DCOLOPRESSO_VALGRIND_TRACK_ORIGINS=ON -DCOLOPRESSO_VALGRIND_RAYON_NUM_THREADS=1`

- Run only a specific Valgrind test:
    - `ctest --test-dir "build" --output-on-failure -R '^test_encode_pngx_memory_valgrind$'`

### Sanitizer Check

1. `rm -rf "build" && cmake -B "build" -DCMAKE_C_COMPILER="$(command -v "clang")" -DCMAKE_CXX_COMPILER="$(command -v "clang++")" -DCMAKE_BUILD_TYPE=Debug -DCOLOPRESSO_USE_(ASAN|MSAN|UBSAN)=ON -DCOLOPRESSO_USE_TESTS=ON`
2. `cmake --build "build" --parallel`
3. `ctest --test-dir "build" --output-on-failure --parallel`

## Build (macOS)

1. Install `cmake` via Homebrew (and `nasm` on Intel CPUs)
2. Move to the clone directory in your terminal
3. `rm -rf "build" && cmake -B "build" -DCMAKE_BUILD_TYPE=Release -DCOLOPRESSO_USE_UTILS=ON -DCOLOPRESSO_USE_TESTS=ON -DCOLOPRESSO_USE_CLI=ON`
4. `cmake --build "build" --parallel`
5. `ctest --test-dir "build" --output-on-failure --parallel`
6. `./build/cli/colopresso` and `./build/utils/cpres` contain executables; `./build` contains `libcolopresso.a`

## Build (Node.js)

1. Install VS Code and Docker (or compatible software), then open the repository directory
2. Attach using Dev Containers
3. `rm -rf "build" && emcmake cmake -B "build" -DCMAKE_BUILD_TYPE=Release -DCOLOPRESSO_USE_UTILS=ON -DCOLOPRESSO_USE_TESTS=ON -DCOLOPRESSO_USE_CLI=ON -DCOLOPRESSO_NODE_BUILD=ON`
4. `cmake --build "build" --parallel`
5. `ctest --test-dir "build" --output-on-failure --parallel`
6. With `COLOPRESSO_USE_UTILS=ON`, Node.js-ready `*.js` / `*.wasm` files are placed under `./build/utils/`; with `COLOPRESSO_USE_CLI=ON`, `./build/cli/colopresso.js` / `colopresso.wasm` are generated alongside `libcolopresso.a`

## Build (Chrome Extension)

1. Install VS Code and Docker (or compatible software), then open the repository directory
2. Attach using Dev Containers
3. `rm -rf "build" && emcmake cmake -B "build" -DCMAKE_BUILD_TYPE=Release -DCOLOPRESSO_CHROME_EXTENSION=ON`
    - File I/O APIs are automatically disabled; only memory-based APIs remain available
4. `cmake --build "build" --parallel`
5. The extension build artifacts are placed under `./build/chrome`

## Build (Electron)

### Common Requirements

- Node.js
- Rust stable with the `wasm32-unknown-emscripten` target
- EMSDK installed at the same tag as `third_party/emsdk`
- `emcmake` / `cmake` accessible via `PATH`
- File I/O APIs are automatically disabled for Electron builds; only memory APIs remain available

### macOS

> [!TIP]
> Refer to `release.yaml` whenever you need the authoritative, up-to-date sequence of steps.

1. In `third_party/emsdk`, run `./emsdk install <tag>` / `./emsdk activate <tag>` and source `. ./emsdk_env.sh`
2. `rm -rf "build" && emcmake cmake -B "build" -DCOLOPRESSO_ELECTRON_APP=ON -DCOLOPRESSO_ELECTRON_TARGETS="--mac"`
3. `cmake --build "build" --config Release --parallel`
4. Artifacts are written to `dist_build/colopresso-<version>_{x64,arm64}.dmg`

### Windows

> [!TIP]
> Always use `pwsh` instead of cmd (Command Prompt).

1. Run `third_party/emsdk\emsdk.ps1 install <tag>` / `activate <tag>` and apply `emsdk_env.ps1`
1. `rm -rf "build" && emcmake cmake -B "build" -DCOLOPRESSO_ELECTRON_APP=ON -DCOLOPRESSO_ELECTRON_TARGETS="--win"`
1. `cmake --build "build" --config Release --parallel`
1. Artifacts appear as `dist_build/colopresso-<version>_{ia32,x64,arm64}.exe`

## License

GNU General Public Licence 3 (GPLv3)

see [`LICENSE`](./LICENSE) for details.

## Authors

- Go Kudo <g-kudo@colopl.co.jp>

- Icon resource designed by Moana Sato

## Notices

See [`NOTICE`](./NOTICE).

This software was developed with the assistance of Large Language Models (LLMs).
All design decisions were made by humans, and all LLM-generated code has been
reviewed and verified for correctness and compliance. 

The icons and graphical assets for this software were created without the use
of generative AI. Files in the assets directory are test images generated
programmatically from Python code.

Coding agents used:

- GitHub, Inc. / GitHub Copilot
- Anthropic PBC / Claude Code
- Anysphere, Inc. / Cursor
