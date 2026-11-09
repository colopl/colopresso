# Changelog

Routine dependency bumps are summarized per release under "Dependencies"; only the versions
that matter for compatibility or security are called out.

Changes are grouped by build type: Library (libcolopresso and its public API), CLI, GUI
(Electron; the Chrome extension until 13.0.0), JS/WASM (Node.js and Emscripten builds), Python,
Build / CI, Documentation and General. Each entry is tagged Added, Changed, Fixed or Removed.
Security fixes are listed first in each release and marked **[SECURITY]**.

## [Unreleased]

### Security
- **[SECURITY]** **Fixed:** GUI: Electron is updated from 44.7.2 to 45.2.3 (Chromium 156.0.8078.80).
  It mitigates the compromised eMudhra intermediate certificates (GHSA-jr5w-w6x2-84p9,
  CVE-2026-115570, high): with earlier versions, an attacker able to intercept network traffic could
  impersonate a TLS server, such as the update server, to the app. It also brings backported upstream
  security fixes from Chromium, V8, ANGLE, Dawn, FFmpeg, PDFium and Skia (#350).
- **[SECURITY]** **Fixed:** Library: zlib is updated from 1.3.2 to 1.4, which fixes memory safety
  issues in inflate, deflate, the gzip functions and minizip (CVE-2026-85091 and others) (#350).
- **[SECURITY]** **Fixed:** Library: the AVIF encoder is built with libaom v3.15.3 instead of v3.14.1,
  which libavif v1.4.2 fetches. libaom v3.15.x fixes memory safety issues found by fuzzing and audits,
  including a heap double-free in multi-threaded encoding. CMake declares libaom before libavif only
  while the libavif submodule pins an older tag, so the override stops applying once a libavif release
  catches up (#350).
- **[SECURITY]** **Fixed:** Library: crossbeam-epoch, which the native builds link through rayon, is
  updated from 0.9.18 to 0.9.21 (RUSTSEC-2026-0204) (#350).
- **[SECURITY]** **Fixed:** GUI: electron-updater is updated from 6.8.9 to 6.8.12. It rejects cached
  update file names that are missing or contain a path, and no longer sends credential headers to
  another origin when a differential download is redirected (#350).
- **[SECURITY]** **Fixed:** Build / CI: pnpm 12.18.0 includes the security fixes of pnpm 12.15.0 and
  12.16.0 (zstd decompression, a timing leak in signing, and packages whose registry metadata names
  another package) (#350).
- **[SECURITY]** **Fixed:** Build / CI: vulnerable transitive build-tool dependencies (undici,
  fast-uri) are updated to patched releases. minimatch, used by electron-builder, still has an
  advisory (GHSA-g3hv-75wg-m249) that is fixed only in minimatch 11, which electron-builder does not
  accept yet. The sprintf-js advisory noted in 14.2.2 has been withdrawn (#350).
- **[SECURITY]** **Fixed:** Build / CI: the pinned `dtolnay/rust-toolchain` action no longer
  interpolates its inputs directly into shell commands (#350).

### GUI
- **Changed:** The GUI runs on Electron 45 (Chromium 156, Node.js 24.22) instead of Electron 44
  (#350).

### JS/WASM
- **Changed:** The Emscripten (Node.js / WebAssembly) builds use Emscripten 7.0.0 (was 6.0.12), whose
  libc++, libunwind and compiler-rt come from LLVM 23.1.3 (#350).

### Build / CI
- **Changed:** Windows builds link zlib 1.4's static library under its new MSVC name, `libzs.lib`
  (was `zs.lib`) (#350).
- **Fixed:** Electron packaging passes the project directory to electron-builder explicitly. On the
  Windows CI runner, electron-builder was started in the build directory and failed with "Cannot find
  package.json" (#350).
- **Removed:** The `electron@44.7.2` exemption from pnpm's `minimumReleaseAge`, which is no longer
  needed (#350).
- **Added:** CI runs a GUI end-to-end smoke test of the packaged Electron app on Windows (x64, arm64)
  and macOS (Intel, Apple Silicon): WebP, AVIF and PNGX conversions through the folder flow, a dropped
  file and a dropped batch saved as a ZIP. Screenshots and logs are uploaded as artifacts.
  `pnpm run e2e:electron` runs it locally, and `playwright-core` is a new devDependency (#352).
- **Changed:** CI runs on macOS 26 (`macos-26`, `macos-26-intel`) and Ubuntu 26.04 (`ubuntu-26.04`,
  `ubuntu-26.04-arm`), and release Linux jobs on Ubuntu 26.04. Release macOS jobs stay on macOS 15
  because the binaries have no deployment target and would otherwise require the newer macOS. CI job
  names label macOS as Intel / Apple Silicon, Windows as x64 / arm64 and Linux as amd64 / arm64 (#352).
- **Changed:** Renovate also updates runner labels set in workflow matrices, except the macOS runners
  of release builds (#352).
- **Changed:** Pull requests that only change the Electron app or shared UI (`app/`), JS dependencies or
  documents skip the library, CLI, WebAssembly and Python wheel jobs; the Electron jobs and their E2E
  still run. Pushes and manual runs always run every job (#352).
- **Changed:** The CI and release Electron jobs cache what Electron and electron-builder download from
  GitHub (Electron itself and tools such as NSIS and the DMG builder), keyed by OS, architecture,
  Electron and electron-builder versions, so builds no longer fail whenever GitHub answers with 504
  Gateway Time-out. The cache is saved right after the build, before tests run. Jobs that build the Rust
  bridge also cache the Cargo registry. A failed Electron architecture no longer cancels the other (#352).
- **Changed:** CI and release jobs no longer clone libaom from aomedia.googlesource.com on every build:
  they restore the checkout of the tag in use into `.cache/libaom/<tag>` from the Actions cache and
  clone it only on a cache miss. CMake builds from such a checkout when it is present. The libaom
  version logic moved to `cmake/libaom.cmake`, which `cmake -P cmake/libaom.cmake` runs to print the
  tag (#352).
- **Changed:** CI and release jobs fetch the Electron zip, its `SHASUMS256.txt` and the
  `virtualenv.pyz` that cibuildwheel uses on Windows and macOS through the GitHub API with the job
  token, and put them where electron-builder and cibuildwheel look first. electron-builder downloaded
  `SHASUMS256.txt` from github.com anonymously on every build, even with the zip cached, and
  cibuildwheel downloaded `virtualenv.pyz` on every run, so both failed while GitHub answered anonymous
  requests with 504 Gateway Time-out. The tools electron-builder downloads (such as NSIS and the DMG
  builder) are still fetched anonymously, but only on a cache miss (#352).

### Documentation
- **Added:** `.claude/skills/rollup-dependencies`, a Claude Code skill (`/rollup-dependencies`) for
  dependency rollups: inventory, security review, updates, builds on each platform, CHANGELOG and
  PR preparation (#350).
- **Added:** `.claude/skills/colopresso-electron-e2e`, a Claude Code skill (`/colopresso-electron-e2e`)
  for driving the packaged Electron app and debugging GUI issues (#352).

### Dependencies
- Electron 45.2.3, electron-updater 6.8.12, electron-builder 26.18.1, Vite 8.3.6, pnpm 12.18.0,
  emsdk 7.0.0, Rust 1.99.1, zlib 1.4, libaom 3.15.3, wasm-pack 0.15.1, wasm-bindgen 0.2.130,
  React 19.3.1, react-redux 9.3.1, `@reduxjs/toolkit` 2.14.0, `@vitejs/plugin-react` 6.2.1, tslib
  2.9.1, Valgrind 3.28.0 (CI), pnpm/action-setup 6.1.1, pypa/gh-action-pypi-publish 1.14.3.

## [14.2.2] - 2026-10-15

### Security
- **[SECURITY]** **Fixed:** GUI: Electron 44.7.2 brings backported upstream security fixes from Chromium
  (152.0.7977.149), V8, ANGLE, Skia, Dawn, WebRTC, PDFium and fontconfig (#337).
- **[SECURITY]** **Fixed:** GUI: js-yaml, which electron-updater bundles to parse update metadata, is
  updated from 4.2.0 to 4.3.2 to fix CPU exhaustion advisories (GHSA-52cp-r559-cp3m,
  GHSA-5p4m-2wfm-xmqj, GHSA-2883-xcg3-v3hh) (#337).
- **[SECURITY]** **Fixed:** Build / CI: vulnerable transitive build-tool dependencies (tar, undici,
  @xmldom/xmldom, fast-uri, http-cache-semantics, brace-expansion, source-map-js) are updated to
  patched releases. sprintf-js still has an open advisory without a fixed release (#337).

### Build / CI
- **Changed:** The CI and release Python wheel jobs run cibuildwheel and the Windows/macOS wheel
  smoke test on Python 3.15 (was 3.14). Published wheels remain `cp310-abi3` (#337).
- **Changed:** `pnpm-workspace.yaml` exempts `electron@44.7.2` from pnpm's one-day
  `minimumReleaseAge` so the security release above can be installed before the window passes. Only
  that exact version is exempted; the entry can be dropped once it ages out (#337).

### Dependencies
- Electron 44.7.2, Vite 8.3.4, pnpm 12.13.0, emsdk 6.0.12, Rust 1.99.0, `@vitejs/plugin-react` 6.1.2,
  `@reduxjs/toolkit` 2.13.0, `@types/node` 24.19.2, Prettier 3.9.10, actions/setup-node 7.1.0.

## [14.2.1] - 2026-09-29

### Security
- **[SECURITY]** **Fixed:** Library: libpng is updated from 1.6.58 to 1.6.59, which fixes
  CVE-2026-46675 (medium; use-after-free of zlib input in `png_read_end` after incomplete zTXt, iTXt
  or iCCP decompression) and a double free of `png_struct` members after an allocation failure
  (#316).
- **[SECURITY]** **Fixed:** GUI: Electron 44.4.5 brings backported upstream security fixes from
  Chromium, V8, ANGLE, Dawn, PDFium and Skia (#289, #309, #311).

### Dependencies
- Electron 44.4.5, Vite 8.3.1, pnpm 12.6.0, emsdk 6.0.10, libpng 1.6.59, wasm-bindgen 0.2.129,
  React 19.3.0, `@types/node` 24.19.0, Prettier 3.9.9, Ubuntu 26.04 (coverage job runner).

## [14.2.0] - 2026-09-10

### Library
- **Fixed:** PNGX (Palette256): the saliency-map option had no effect while post-process
  smoothing was enabled, because the importance map built for smoothing was always handed to the
  quantizer. The map now reaches libimagequant only when the saliency map is enabled, and a
  regression test guards the behavior (#298).
- **Fixed:** Emscripten log path: the `#if` guard mixed a runtime variable into a preprocessor
  condition; it now checks build flags only (#298).

### CLI
- **Added:** PNGX quality toggles that were GUI-only can now be set from the command line:
  `--[no-]saliency-map`, `--[no-]chroma-anchor`, `--[no-]adaptive-dither`,
  `--[no-]gradient-boost`, `--[no-]chroma-weight` and `--[no-]smooth`. Defaults follow the
  library defaults, and help text now derives defaults from the public header (#298).
- **Changed:** The CLI no longer includes internal library headers; boolean toggles are table
  driven (#298).
- **Changed:** With `--type limited`, the saliency map, chroma anchor, adaptive dither, gradient
  boost, chroma weight and smoothing options are always forced off because Limited RGBA4444 never
  uses them. Their `--no-*` forms are accepted; enabling any of them (`--saliency-map`,
  `--chroma-anchor`, `--adaptive-dither`, `--gradient-boost`, `--chroma-weight`, `--smooth`) is
  rejected with an error unless a later `--no-*` turns it off again.

### GUI
- **Changed:** Selecting Limited RGBA4444 in the advanced settings disables the saliency map,
  high chroma anchor, adaptive dithering, gradient boost, chroma weighting, post-process smoothing
  and smoothing cutoff controls and explains that they apply only to the 256-color palette and
  Reduced RGBA32 modes.

### Python
- **Changed:** README documents all Palette256 profile/tune parameters, protected colors and the
  Reduced RGBA32 parameters that were previously missing or misplaced (#298).

### Build / CI
- **Fixed:** Incremental CMake builds did not rebuild `pngx_bridge` when only `Cargo.lock`
  changed, so stale Rust crates could remain linked. The lock file is now part of the staging
  inputs (#298).
- **Changed:** pnpm 12.3.4 is the pinned package manager (`packageManager`); the Dockerfile
  installs the pinned version and the lock file records pnpm's own dependency block (#298).
- **Changed:** Renovate: removed the custom Rust toolchain regex manager that duplicated the
  built-in `rust-toolchain` manager (#298).
- **Changed:** Renovate: npm updates wait one day after publication (`minimumReleaseAge`), matching
  pnpm's default supply-chain policy so generated lockfiles pass `pnpm install --frozen-lockfile`.

### Documentation
- **Added:** `CHANGELOG.md` and `CHANGELOG_ja.md`; `AGENTS.md` requires notable changes to be
  recorded in their Unreleased sections.
- **Changed:** `AGENTS.md` describes the full repository layout, formatting rules and the
  location of default values (#298).

### Dependencies
- Rust 1.98.1, oxipng 10.2.1, wasm-bindgen 0.2.128, emsdk 6.0.9, Electron 44.3.0,
  `@vitejs/plugin-react` 6.1.1, `@types/react-dom` 19.2.7, pnpm/action-setup 6.1.0.

## [14.1.2] - 2026-08-31

### GUI
- **Added:** Detects an x64 build running under ARM64 translation (Rosetta 2 on macOS, x64
  emulation on Windows on ARM), shows a warning with the native architecture, and treats the
  same version's native build as an available update so the app can switch automatically (#288).
- **Fixed:** PNGX regression: option values persisted as strings (from `<select>` elements or
  imported profiles) were passed through unchanged, so numeric and boolean PNGX settings could
  be ignored. Schema-based coercion now normalizes every backend's input (#288).
- **Fixed:** Native addon: numeric options given as strings are parsed, integer ranges are
  checked before casting, and thread counts are validated (#288).

### Build / CI
- **Added:** Windows update metadata generator (`utils/create_windows_update_metadata.mjs`)
  with tests, so per-architecture `latest.yml` files are published like the macOS ones (#288).
- **Added:** Node-API addon test suite (`colopresso_native.test.mjs`) and a CI step that runs
  it (#288).

### Dependencies
- Electron 44, Vite 8.2.2, pnpm 11.24.0, emsdk 6.0.8, Rust 1.98.0, `@vitejs/plugin-react` 6.1.0.

## [14.1.0] - 2026-08-14

### Build / CI
- **Changed:** Valgrind is no longer a git submodule; the CI image builds it from the release
  tarball and Renovate tracks new versions through a custom datasource (#266).
- **Changed:** Reverted a `pypa/gh-action-pypi-publish` bump that broke publishing, then
  re-applied 1.14.2 (#275).

### Dependencies
- Electron 43.4.0, Rust 1.97.1, oxipng 10.2.0, wasm-bindgen 0.2.127, emsdk 6.0.5, Unity 2.7.0,
  Vite 8.2.1, Rollup 5, pnpm 11.13.0, actions/setup-node v7 (#264, #265, #275).

## [14.0.1] - 2026-07-14

### GUI
- **Fixed:** Auto-update on macOS arm64 picked the wrong artifact. A new
  `utils/create_macos_update_metadata.mjs` script (with tests) writes per-architecture
  `latest-mac.yml` files during release (#252).

## [14.0.0] - 2026-07-14

### GUI
- **Added:** WebP, AVIF and PNG are encoded through a native Node-API addon
  (`colopresso_native.node`) built from libcolopresso, instead of WebAssembly (#240).
- **Added:** Settings for the conversion thread count (default: half of the available threads)
  and for disabling multithreaded WebP encoding (#240).
- **Changed:** Packages are built per architecture (`COLOPRESSO_ELECTRON_ARCH`); AVX2 is now
  required on x86_64 for the Electron build as well as CLI and Python wheels. Linux Electron
  packaging is not supported (#240).

### JS/WASM
- **Removed:** The separated nightly `wasm-bindgen` build mode. WebAssembly remains for the
  Node.js build and as a legacy Electron fallback (#249).

### Build / CI
- **Added:** CMake modules `cmake/electron_app.cmake` and `cmake/electron_native_addon.cmake`
  for the native Electron build (#240).
- **Removed:** Rust nightly toolchain files and the MemorySanitizer build option (#249).

### Dependencies
- emsdk 6, cibuildwheel 4, electron-builder 26.15.3, Electron 42.4.1, wasm-bindgen 0.2.126,
  cbindgen 0.29.4, actions/checkout v7, Alpine 3.24 (#241).

## [13.0.6] - 2026-06-11

### Dependencies
- Electron 42.4.0, electron-updater 6.8.9, electron-builder 26.15.2.

## [13.0.5] - 2026-06-08

### Build / CI
- **Changed:** Workflows triggered by Renovate pull requests now wait for manual approval
  through a protected environment before running.

### Dependencies
- Rust 1.96.0, cbindgen 0.29.3, Electron 42.3.3, Vite 8.0.16.

## [13.0.4] - 2026-05-28

### JS/WASM
- **Fixed:** The nightly WebAssembly build exports `__heap_base` (#216).

### Build / CI
- **Fixed:** macOS Electron packaging emits dmg and zip for arm64 and x64 through separate
  electron-builder invocations; the `electron-winstaller` build script is disallowed in
  `pnpm-workspace.yaml` (#216).

### Dependencies
- libavif 1.4.2, Electron 42.3.0.

## [13.0.2] - 2026-05-21

### Dependencies
- Electron 42, wasm-pack 0.15.0, react-redux 9.3.0, `@reduxjs/toolkit` 2.12.0, Vite 8.0.13,
  Valgrind 3.27.1, emsdk 5.0.7, React 19.2.6.

## [13.0.1] - 2026-04-27

### JS/WASM
- **Changed:** Build documentation describes the Emscripten build matrix: Node.js uses the
  integrated stable `wasm32-unknown-emscripten` `pngx_bridge` (Rayon disabled, `panic=abort`),
  while Electron used the separated nightly `pngx_bridge.js` / `pngx_bridge_bg.wasm` assets with
  `wasm-bindgen-rayon` threading.

### Dependencies
- Rust 1.95.0, oxipng 10.1.1, Electron 41.3.0, Valgrind 3.27.0, pnpm/action-setup v6, Vite 8.0.10.

## [13.0.0] - 2026-04-20

### GUI
- **Removed:** Chrome extension (`app/chrome`) and its shared-code hooks; the GUI is Electron
  only (#165).

### Build / CI
- **Changed:** Toolchain moved to LLVM/clang 22; Electron build fixes and Rust nightly bump (#165).
- **Added:** Renovate configuration in the repository (#165).

### General
- **Changed:** License changed from GPL-3.0 to GPL-3.0-or-later (#165).

### Dependencies
- Electron 41.2.1, libpng 1.6.58, TypeScript 6.0.3, rayon 1.12.0, emsdk 5.0.6, Vite 8.0.8.

## [12.3.3] - 2026-04-09

### Dependencies
- Vite 8.0.5 (security) then 8.0.7, Electron 41.2.0, React 19.2.5, libpng 1.6.57,
  Debian trixie-20260406 base image.

## [12.3.1] - 2026-04-06

### Dependencies
- emsdk 5.0.5, `@types/node` 24.12.2, `@types/chrome` 0.1.39.

## [12.3.0] - 2026-04-03

### GUI
- **Changed:** Settings that require a restart now relaunch the application
  (`app.relaunch()`) instead of reloading the window, and the pending update check survives
  the restart through `localStorage`. UI wording changed from "reload" to "restart" (#143).

### Dependencies
- Electron 41.1.1, lodash 4.18.1 and `@xmldom/xmldom` 0.8.12 (security).

## [12.2.4] - 2026-03-30

### Build / CI
- **Fixed:** Test timeouts and the coverage target.

### Dependencies
- Electron 41.1.0, libpng 1.6.56, Vite 8.0.3, actions/deploy-pages v5, actions/configure-pages v6.

## [12.2.3] - 2026-03-24

### GUI
- **Changed:** The Electron preload script is bundled by a dedicated Vite config
  (`vite.preload.config.ts`).

### Build / CI
- **Changed:** Valgrind test timeouts raised.
- **Changed:** zlib 1.3.2 required updated CMake options (`ZLIB_BUILD_TESTING`, `ZLIB_INSTALL`,
  new static library name on Windows).

### Dependencies
- Vite 8, TypeScript 6, `@vitejs/plugin-react` 6, Electron 41.0.3, libavif 1.4.1, zlib 1.3.2,
  emsdk 5.0.4, pnpm/action-setup v5; security bumps for tar, rollup, ajv, minimatch, picomatch.

## [12.2.2] - 2026-02-12

### Dependencies
- Electron 40.4.0, libpng 1.6.55, `@vitejs/plugin-react` 5.1.4.

## [12.2.1] - 2026-02-05

### Build / CI
- **Changed:** Rust nightly 1.95 for the wasm-bindgen build, LLVM apt repository trust handling,
  explicit rustup default when installing `wasm-bindgen-cli` (#81).

### Dependencies
- emsdk 5, Electron 40.1.0, electron-builder 26.7.0, React monorepo update.

## [12.2.0] - 2026-01-21

### Library
- **Changed:** PNGX: `pngx_threads = 0` now means automatic thread count.
- **Changed:** Palette256 alpha bleed is disabled by default
  (`COLOPRESSO_PNGX_DEFAULT_PALETTE256_ALPHA_BLEED_ENABLE` is `false`); GUI and Python defaults
  follow (#71).

### GUI
- **Fixed:** Memory leak in the PNGX encoder path (#66).
- **Fixed:** Cancelling a conversion no longer waits for in-flight work to finish (#66).
- **Fixed:** The custom protocol handler builds URLs with `pathToFileURL` and rejects paths
  outside the application directory (path traversal returns 403) (#66).

### Python
- **Added:** Python bindings (`python/`), published as wheels for Windows, macOS and Linux
  (x64/arm64) using the CPython stable ABI. `pip install colopresso` exposes WebP, AVIF and PNGX
  encoding with a `Config` dataclass mirroring the C configuration (#66).

### Build / CI
- **Changed:** Valgrind runs in a separate job.

### Documentation
- **Changed:** README states the AVX2 requirement for native x86_64 builds (CLI, Python wheels).

### Dependencies
- Python 3.14 and cibuildwheel 3 in CI, React types updates.

## [12.1.1] - 2026-01-16

### Dependencies
- Electron 40, libpng 1.6.54, emsdk 4.0.23, Prettier 3.8.0, electron-updater 6.7.3, Vite 7.3.1.

## [12.1.0] - 2025-12-26

### Library
- **Added:** Public API: `cpres_get_compiler_version_string`, `cpres_get_rust_version_string`,
  `cpres_is_threads_enabled`, `cpres_get_default_thread_count`, `cpres_get_max_thread_count`.
- **Changed:** SIMD acceleration improves overall performance; portable mutex fix (#52).

### GUI
- **Added:** PNG (Palette256) multithreading option, conversion cancel button (Esc), per-file
  conversion time in milliseconds, "environment settings" section with restart-required
  warnings, and an update check when settings are reset. Conversion time and cancel are also
  available in the Chrome extension (#52).
- **Changed:** GUI restructured around shared React components, hooks and a conversion worker;
  the `pngx_bridge` WebAssembly module is loaded through a dedicated bridge (#52).
- **Fixed:** Windows GUI startup fixes.

### Build / CI
- **Fixed:** Release workflow hotfix.

### Documentation
- **Changed:** README rewritten with a feature overview, format guide and per-application
  sections (#52).

## [12.0.0] - 2025-12-22

### Library
- **Added:** PNGX Palette256 tuning options: gradient-profile auto tuning with dither floor,
  alpha bleed (max distance, opaque threshold, soft limit), profile thresholds (opaque ratio,
  gradient mean, saturation mean) and auto-tune limits (speed max, quality floor/target), all
  exposed in `cpres_config_t` (#36).
- **Added:** `COLOPRESSO_WITH_FILE_OPS` CMake option to build without file I/O APIs; `datetime`
  and portability headers moved under `library/include/colopresso/` (#47).
- **Changed:** PNGX Palette256 and Limited RGBA4444 quality improvements; internal encoder APIs
  refactored and `pngx_limited4444.c` renamed to `pngx_limited.c` (#36, #42, #50).

### CLI
- **Added:** Palette256 options `--gradient-profile`, `--alpha-bleed*`, `--gradient-*` and
  `--tune-*` (#36).

### GUI
- **Added:** Palette256 tuning options in the advanced settings panel (#36).
- **Changed:** Auto-update flow: click to download, then restart to install (#42, #49).

### Build / CI
- **Changed:** Valgrind runs only on `main` after CI passed despite memory errors (#44, #47);
  MemorySanitizer suppressions removed in favor of Valgrind suppressions.

### Documentation
- **Added:** Valgrind runner options (`COLOPRESSO_VALGRIND_*`) documented in README (#47).
- **Changed:** Download page improvements (#49).

### Dependencies
- electron-builder 26.4.0, emsdk 4.0.22, Vite 7.3.0, Electron 39.2.7, GitHub artifact actions.

## [11.0.1] - 2025-12-12

### GUI
- **Fixed:** Auto-update: download/extract progress reporting and install flow (#35).
- **Fixed:** Original PNG files were not removed after PNGX conversion (#35).
- **Changed:** Application name unified as "colopresso" (was "Colopresso") (#35).

### Build / CI
- **Fixed:** Release workflow for the Windows GUI; unused artifacts removed and macOS zip build
  disabled (#35).

### Documentation
- **Fixed:** Download page layout and default language (#35).

### Dependencies
- electron-builder 26.3.6.

## [11.0.0] - 2025-12-10

### Library
- **Fixed:** Reported oxipng and libimagequant versions were wrong; versions are now read at
  build time by `build.rs` (#29, #30).
- **Changed:** `konst` crate removed from `pngx_bridge`.

### GUI
- **Added:** Auto-update through `electron-updater`, with update checks every 6 hours (#23, #28).
- **Fixed:** "Delete original file" did not work when the output format was PNG (#27, #28).

### Build / CI
- **Changed:** claude-code extension removed from the devcontainer.

### Documentation
- **Added:** GitHub Pages download page (`pages/index.html`) (#28).

### Dependencies
- Vite 7.2.7, `@reduxjs/toolkit` 2.11.1, `@vitejs/plugin-react` 5.1.2, `@types/node` 24.10.2.

## [10.0.2] - 2025-12-08

### Security
- **[SECURITY]** **Fixed:** libpng updated to 1.6.53 to fix CVE-2025-66293.

### Dependencies
- oxipng 10, Electron 39.2.6, electron-builder 26.3.5.

## [10.0.1] - 2025-12-05

### Build / CI
- **Changed:** Intel macOS jobs use the `macos-15-intel` runner and the latest NASM.

### Documentation
- **Added:** Project icon and screenshots in README.

### Dependencies
- Electron 39.2.5, electron-builder 26.3.4, libpng 1.6.52, emsdk 4.0.21, React 19.2.1,
  Vite 7.2.6, Prettier 3.7.4, Alpine 3.23.

## [10.0.0] - 2025-11-26

Initial public release.

### Library
- `libcolopresso`: C99 library converting PNG to WebP (lossy/lossless), AVIF (lossy/lossless)
  and optimized PNG. PNG modes: lossless optimization (metadata removal), 256-color palette
  quantization with protected colors, Reduced RGBA32 (auto or manual color targets, RGB/alpha
  grid bit depth 1-8, importance-aware dithering) and Limited RGBA4444 with error-diffusion
  dithering and auto-dither heuristics.
- `pngx_bridge`: Rust bridge to oxipng and libimagequant.

### CLI
- Command-line converter for Windows, Linux and macOS using OS-level multithreading and CPU
  extensions.

### GUI
- Electron desktop app: converts every PNG in a dropped or selected folder, optional deletion
  of originals, profile support.
- Chrome extension: direct downloads from the browser, optional ZIP archive for multiple files.

### JS/WASM
- Node.js WebAssembly build of the CLI (`colopresso.js` / `colopresso.wasm`).

### General
- Profile system to save, export and import per-format parameters.
