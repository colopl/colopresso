# 変更履歴

定型的な依存関係の更新は各リリースの「Dependencies」にまとめ、互換性やセキュリティに関わるものだけを
明記しています。

変更はビルドタイプごとに分類しています: Library (libcolopresso と公開 API)、CLI、GUI (Electron。
13.0.0 までは Chrome 拡張を含む)、JS/WASM (Node.js / Emscripten ビルド)、Python、Build / CI、
Documentation、General。各項目には 追加 / 変更 / 修正 / 削除 のいずれかを付けています。

## [14.2.0] - 2026-09-10

### Library
- **修正:** PNGX (Palette256): ポストプロセス平滑化が有効な間、視覚重要度マップのオプションが効いていませんでした。
  平滑化のために生成した重要度マップを無条件に量子化器へ渡していたためです。視覚重要度マップが有効な
  時だけ libimagequant へ渡すようにし、回帰テストを追加しました (#298)。
- **修正:** Emscripten 向けログ処理の `#if` 条件に実行時変数が含まれていたため、ビルドフラグのみを
  判定する形に修正しました (#298)。

### CLI
- **追加:** GUI でしか指定できなかった PNGX の品質トグルをコマンドラインから指定できるようになりました。
  `--[no-]saliency-map`, `--[no-]chroma-anchor`, `--[no-]adaptive-dither`, `--[no-]gradient-boost`,
  `--[no-]chroma-weight`, `--[no-]smooth` です。デフォルトはライブラリと同じで、ヘルプ表示の既定値は
  公開ヘッダのマクロから導出されます (#298)。
- **変更:** CLI がライブラリ内部ヘッダを参照しないようにし、真偽値トグルをテーブル駆動にしました (#298)。
- **変更:** `--type limited` では、視覚重要度マップ・高彩度アンカー・適応ディザリング・グラデーションブースト・
  彩度重み付け・平滑化を常に強制的に無効にします。Limited RGBA4444 はこれらを一切使用しないためです。`--no-*`
  形式は受け付け、有効化オプション (`--saliency-map`, `--chroma-anchor`, `--adaptive-dither`,
  `--gradient-boost`, `--chroma-weight`, `--smooth`) は、後続の `--no-*` で打ち消されない限りエラーにします。

### GUI
- **変更:** PNG 詳細設定で `Limited RGBA4444` を選んだ場合、利用不能な `視覚重要度マップ`, `高彩度アンカー`,
  `適応ディザリング`, `グラデーションブースト`, `彩度重み付け`, `ポストプロセス平滑化`, `平滑化重要度しきい値` の
  入力を無効化 (グレーアウト) し、これらが 256 色パレットと Reduced RGBA32 でのみ有効であることを注記として
  表示するようにしました。

### Python
- **変更:** README に記載漏れだった Palette256 のプロファイル / 自動チューニング項目、保護色、
  Reduced RGBA32 の項目を追記し、誤っていた分類を修正しました (#298)。

### Build / CI
- **修正:** CMake の差分ビルドで `Cargo.lock` だけが変わった場合に `pngx_bridge` が再ビルドされず、
  古い Rust クレートがリンクされたままになる問題を修正しました。ロックファイルをステージ対象の依存に
  追加しています (#298)。
- **変更:** パッケージマネージャを pnpm 12.3.4 に固定しました (`packageManager`)。Dockerfile も
  固定バージョンを導入し、ロックファイルに pnpm 自身の依存ブロックが記録されます (#298)。
- **変更:** Renovate: 組み込みの `rust-toolchain` マネージャと重複していたカスタム regex マネージャを
  削除しました (#298)。
- **変更:** Renovate: npm の更新は公開から 1 日待ってから提案するようにしました (`minimumReleaseAge`)。pnpm の
  既定のサプライチェーン検査に合わせ、生成されたロックファイルが `pnpm install --frozen-lockfile` を通るようにするためです。

### Documentation
- **追加:** `CHANGELOG.md` と `CHANGELOG_ja.md` を追加しました。`AGENTS.md` で、記載が必要な変更は
  両ファイルの Unreleased 節へ追記するよう定めています。
- **変更:** `AGENTS.md` にリポジトリ構成の全体、フォーマット規約、デフォルト値の定義場所を記載しました (#298)。

### Dependencies
- Rust 1.98.1, oxipng 10.2.1, wasm-bindgen 0.2.128, emsdk 6.0.9, Electron 44.3.0,
  `@vitejs/plugin-react` 6.1.1, `@types/react-dom` 19.2.7, pnpm/action-setup 6.1.0。

## [14.1.2] - 2026-08-31

### GUI
- **追加:** x64 ビルドが ARM64 変換環境 (macOS の Rosetta 2、Windows on ARM の x64 エミュレーション)
  で動作していることを検出し、ネイティブアーキテクチャを示す警告を表示します。同一バージョンの
  ネイティブビルドを更新として扱い、自動的に切り替えられるようにしました (#288)。
- **修正:** PNGX のリグレッション: `<select>` 要素やインポートしたプロファイルから文字列として保存された
  設定値がそのまま渡され、数値や真偽値の PNGX 設定が無視されることがありました。スキーマに基づく型変換で
  すべてのバックエンドに同じ設定が渡るようにしました (#288)。
- **修正:** ネイティブアドオン: 文字列で渡された数値の解釈、キャスト前の整数範囲チェック、スレッド数の
  検証を追加しました (#288)。

### Build / CI
- **追加:** Windows 向け更新メタデータ生成スクリプト (`utils/create_windows_update_metadata.mjs`) と
  テストを追加し、macOS と同様にアーキテクチャ別の `latest.yml` を公開します (#288)。
- **追加:** Node-API アドオンのテスト (`colopresso_native.test.mjs`) と、それを実行する CI ステップを
  追加しました (#288)。

### Dependencies
- Electron 44, Vite 8.2.2, pnpm 11.24.0, emsdk 6.0.8, Rust 1.98.0, `@vitejs/plugin-react` 6.1.0。

## [14.1.0] - 2026-08-14

### Build / CI
- **変更:** Valgrind を git submodule から外し、CI イメージではリリース tarball からビルドするようにしました。
  新バージョンは Renovate のカスタム datasource で追跡します (#266)。
- **変更:** 公開処理を壊した `pypa/gh-action-pypi-publish` の更新を一度取り消し、その後 1.14.2 を
  再適用しました (#275)。

### Dependencies
- Electron 43.4.0, Rust 1.97.1, oxipng 10.2.0, wasm-bindgen 0.2.127, emsdk 6.0.5, Unity 2.7.0,
  Vite 8.2.1, Rollup 5, pnpm 11.13.0, actions/setup-node v7 (#264, #265, #275)。

## [14.0.1] - 2026-07-14

### GUI
- **修正:** macOS arm64 の自動更新が誤った成果物を選択していました。リリース時にアーキテクチャ別の
  `latest-mac.yml` を書き出す `utils/create_macos_update_metadata.mjs` (テスト付き) を追加しました (#252)。

## [14.0.0] - 2026-07-14

### GUI
- **追加:** WebP / AVIF / PNG の変換が、WebAssembly ではなく libcolopresso から生成した Node-API
  ネイティブアドオン (`colopresso_native.node`) で行われるようになりました (#240)。
- **追加:** 変換スレッド数 (既定は利用可能スレッドの半分) と、WebP のマルチスレッドエンコード無効化の
  設定を追加しました (#240)。
- **変更:** パッケージはアーキテクチャごとにビルドします (`COLOPRESSO_ELECTRON_ARCH`)。x86_64 では CLI と
  Python Wheel に加えて Electron ビルドでも AVX2 が必須になりました。Linux 向け Electron パッケージは
  未対応です (#240)。

### JS/WASM
- **削除:** 分離型の nightly `wasm-bindgen` ビルドモードを削除しました。WebAssembly は Node.js ビルドと
  Electron のレガシーフォールバックとして残ります (#249)。

### Build / CI
- **追加:** ネイティブ Electron ビルド用の CMake モジュール `cmake/electron_app.cmake` と
  `cmake/electron_native_addon.cmake` を追加しました (#240)。
- **削除:** Rust nightly ツールチェーン定義と MemorySanitizer のビルドオプションを削除しました (#249)。

### Dependencies
- emsdk 6, cibuildwheel 4, electron-builder 26.15.3, Electron 42.4.1, wasm-bindgen 0.2.126,
  cbindgen 0.29.4, actions/checkout v7, Alpine 3.24 (#241)。

## [13.0.6] - 2026-06-11

### Dependencies
- Electron 42.4.0, electron-updater 6.8.9, electron-builder 26.15.2。

## [13.0.5] - 2026-06-08

### Build / CI
- **変更:** Renovate の Pull Request から起動するワークフローは、保護された environment による手動承認を
  待ってから実行されるようになりました。

### Dependencies
- Rust 1.96.0, cbindgen 0.29.3, Electron 42.3.3, Vite 8.0.16。

## [13.0.4] - 2026-05-28

### JS/WASM
- **修正:** nightly WebAssembly ビルドで `__heap_base` をエクスポートするようにしました (#216)。

### Build / CI
- **修正:** macOS の Electron パッケージングで arm64 と x64 の dmg / zip を electron-builder の個別呼び出しで
  生成するようにし、`pnpm-workspace.yaml` で `electron-winstaller` のビルドスクリプトを禁止しました (#216)。

### Dependencies
- libavif 1.4.2, Electron 42.3.0。

## [13.0.2] - 2026-05-21

### Dependencies
- Electron 42, wasm-pack 0.15.0, react-redux 9.3.0, `@reduxjs/toolkit` 2.12.0, Vite 8.0.13,
  Valgrind 3.27.1, emsdk 5.0.7, React 19.2.6。

## [13.0.1] - 2026-04-27

### JS/WASM
- **変更:** ビルドドキュメントに Emscripten ビルドマトリクスを記載しました。Node.js は統合型の安定版
  `wasm32-unknown-emscripten` `pngx_bridge` (Rayon 無効, `panic=abort`) を使い、Electron は
  `wasm-bindgen-rayon` によるスレッド対応の分離型 nightly `pngx_bridge.js` / `pngx_bridge_bg.wasm` を
  使う構成です。

### Dependencies
- Rust 1.95.0, oxipng 10.1.1, Electron 41.3.0, Valgrind 3.27.0, pnpm/action-setup v6, Vite 8.0.10。

## [13.0.0] - 2026-04-20

### GUI
- **削除:** Chrome 拡張 (`app/chrome`) と共有コード側の対応を削除し、GUI は Electron のみになりました (#165)。

### Build / CI
- **変更:** ツールチェーンを LLVM / clang 22 に移行し、Electron ビルドの修正と Rust nightly の更新を
  行いました (#165)。
- **追加:** Renovate の設定ファイルをリポジトリに追加しました (#165)。

### General
- **変更:** ライセンスを GPL-3.0 から GPL-3.0-or-later に変更しました (#165)。

### Dependencies
- Electron 41.2.1, libpng 1.6.58, TypeScript 6.0.3, rayon 1.12.0, emsdk 5.0.6, Vite 8.0.8。

## [12.3.3] - 2026-04-09

### Dependencies
- Vite 8.0.5 (セキュリティ) から 8.0.7, Electron 41.2.0, React 19.2.5, libpng 1.6.57,
  Debian trixie-20260406 ベースイメージ。

## [12.3.1] - 2026-04-06

### Dependencies
- emsdk 5.0.5, `@types/node` 24.12.2, `@types/chrome` 0.1.39。

## [12.3.0] - 2026-04-03

### GUI
- **変更:** 再起動が必要な設定変更で、ウィンドウの再読み込みではなくアプリケーション本体を再起動
  (`app.relaunch()`) するようになりました。保留中の更新確認は `localStorage` を介して再起動後も
  引き継がれます。UI の文言を「再読み込み」から「再起動」に変更しました (#143)。

### Dependencies
- Electron 41.1.1、lodash 4.18.1 と `@xmldom/xmldom` 0.8.12 (セキュリティ)。

## [12.2.4] - 2026-03-30

### Build / CI
- **修正:** テストのタイムアウトとカバレッジターゲットを修正しました。

### Dependencies
- Electron 41.1.0, libpng 1.6.56, Vite 8.0.3, actions/deploy-pages v5, actions/configure-pages v6。

## [12.2.3] - 2026-03-24

### GUI
- **変更:** Electron の preload スクリプトを専用の Vite 構成 (`vite.preload.config.ts`) でバンドルするようにしました。

### Build / CI
- **変更:** Valgrind テストのタイムアウトを延長しました。
- **変更:** zlib 1.3.2 に合わせて CMake オプションを更新しました (`ZLIB_BUILD_TESTING`, `ZLIB_INSTALL`,
  Windows での静的ライブラリ名の変更)。

### Dependencies
- Vite 8, TypeScript 6, `@vitejs/plugin-react` 6, Electron 41.0.3, libavif 1.4.1, zlib 1.3.2, emsdk 5.0.4,
  pnpm/action-setup v5。tar, rollup, ajv, minimatch, picomatch のセキュリティ更新。

## [12.2.2] - 2026-02-12

### Dependencies
- Electron 40.4.0, libpng 1.6.55, `@vitejs/plugin-react` 5.1.4。

## [12.2.1] - 2026-02-05

### Build / CI
- **変更:** wasm-bindgen ビルド向けに Rust nightly 1.95 を使用、LLVM apt リポジトリの信頼設定、
  `wasm-bindgen-cli` インストール時の rustup default 明示 (#81)。

### Dependencies
- emsdk 5, Electron 40.1.0, electron-builder 26.7.0, React 関連の更新。

## [12.2.0] - 2026-01-21

### Library
- **変更:** PNGX: `pngx_threads = 0` がスレッド数の自動決定を意味するようになりました。
- **変更:** Palette256 のアルファブリードを既定で無効にしました
  (`COLOPRESSO_PNGX_DEFAULT_PALETTE256_ALPHA_BLEED_ENABLE` が `false`)。GUI と Python の既定値も
  追従しています (#71)。

### GUI
- **修正:** PNGX エンコード経路のメモリリークを修正しました (#66)。
- **修正:** 変換のキャンセルが実行中の処理の完了を待たなくなりました (#66)。
- **修正:** カスタムプロトコルハンドラが `pathToFileURL` で URL を構築し、アプリケーションディレクトリ外の
  パスを拒否するようになりました (パストラバーサルは 403 を返します) (#66)。

### Python
- **追加:** Python バインディング (`python/`) を追加しました。CPython の stable ABI を用い、
  Windows / macOS / Linux (x64 / arm64) 向け Wheel として公開されます。`pip install colopresso` で
  WebP, AVIF, PNGX のエンコードが利用でき、C の設定構造体に対応する `Config` データクラスを提供します (#66)。

### Build / CI
- **変更:** Valgrind を別ジョブに分離しました。

### Documentation
- **変更:** README にネイティブ x86_64 ビルド (CLI, Python Wheel) の AVX2 必須要件を記載しました。

### Dependencies
- CI で Python 3.14 と cibuildwheel 3 を使用、React 型定義の更新。

## [12.1.1] - 2026-01-16

### Dependencies
- Electron 40, libpng 1.6.54, emsdk 4.0.23, Prettier 3.8.0, electron-updater 6.7.3, Vite 7.3.1。

## [12.1.0] - 2025-12-26

### Library
- **追加:** 公開 API: `cpres_get_compiler_version_string`, `cpres_get_rust_version_string`,
  `cpres_is_threads_enabled`, `cpres_get_default_thread_count`, `cpres_get_max_thread_count`。
- **変更:** SIMD による全体的な性能向上と、移植性のあるミューテックスの修正 (#52)。

### GUI
- **追加:** PNG (Palette256) のマルチスレッドオプション、変換のキャンセルボタン (Esc)、ファイルごとの
  変換時間 (ミリ秒) 表示、再起動が必要な設定の警告を含む「環境設定」セクション、設定リセット時の
  更新確認。変換時間の表示とキャンセルは Chrome 拡張でも利用できます (#52)。
- **変更:** GUI を共通の React コンポーネント、フック、変換ワーカーを中心に再構成しました。`pngx_bridge` の
  WebAssembly モジュールは専用ブリッジ経由で読み込みます (#52)。
- **修正:** Windows GUI の起動を修正しました。

### Build / CI
- **修正:** リリースワークフローのホットフィックス。

### Documentation
- **変更:** README を機能概要、フォーマットガイド、アプリケーション別の説明で全面的に書き直しました (#52)。

## [12.0.0] - 2025-12-22

### Library
- **追加:** PNGX Palette256 のチューニングオプション: ディザ下限付きのグラデーションプロファイル自動調整、
  アルファブリード (最大距離、不透明しきい値、ソフト上限)、プロファイルしきい値 (不透明率、
  グラデーション平均、彩度平均)、自動チューニングの上限 (速度上限、品質下限 / 目標)。いずれも
  `cpres_config_t` から指定できます (#36)。
- **追加:** ファイル I/O API なしでビルドするための `COLOPRESSO_WITH_FILE_OPS` CMake オプション。`datetime` と
  移植性ヘッダを `library/include/colopresso/` 配下に移動しました (#47)。
- **変更:** PNGX Palette256 と Limited RGBA4444 の画質を改善しました。エンコーダ内部 API を
  リファクタリングし、`pngx_limited4444.c` を `pngx_limited.c` に改名しました (#36, #42, #50)。

### CLI
- **追加:** Palette256 向けオプション `--gradient-profile`, `--alpha-bleed*`, `--gradient-*`, `--tune-*` (#36)。

### GUI
- **追加:** 詳細設定パネルに Palette256 のチューニングオプションを追加しました (#36)。
- **変更:** 自動更新の流れを「クリックでダウンロード、再起動でインストール」に変更しました (#42, #49)。

### Build / CI
- **変更:** Valgrind がメモリエラーを検出していても CI が通ってしまう問題 (#44) を受け、Valgrind は `main`
  ブランチでのみ実行するようにしました (#47)。MemorySanitizer の抑制ファイルを削除し、Valgrind の
  抑制ファイルに集約しています。

### Documentation
- **追加:** Valgrind 実行オプション (`COLOPRESSO_VALGRIND_*`) を README に記載しました (#47)。
- **変更:** ダウンロードページを改善しました (#49)。

### Dependencies
- electron-builder 26.4.0, emsdk 4.0.22, Vite 7.3.0, Electron 39.2.7, GitHub artifact actions。

## [11.0.1] - 2025-12-12

### GUI
- **修正:** 自動更新のダウンロード / 展開の進捗表示とインストールの流れを修正しました (#35)。
- **修正:** PNGX 変換後に元の PNG ファイルが削除されない問題を修正しました (#35)。
- **変更:** アプリケーション名を「colopresso」に統一しました (旧「Colopresso」) (#35)。

### Build / CI
- **修正:** Windows GUI のリリースワークフローを修正し、未使用の成果物を削除、macOS の zip ビルドを
  無効化しました (#35)。

### Documentation
- **修正:** ダウンロードページのレイアウトと既定言語を修正しました (#35)。

### Dependencies
- electron-builder 26.3.6。

## [11.0.0] - 2025-12-10

### Library
- **修正:** oxipng と libimagequant の表示バージョンが誤っていた問題を修正し、`build.rs` でビルド時に
  取得するようにしました (#29, #30)。
- **変更:** `pngx_bridge` から `konst` クレートを削除しました。

### GUI
- **追加:** `electron-updater` による自動更新。更新確認は 6 時間ごとに行われます (#23, #28)。
- **修正:** 出力形式が PNG の時に「元ファイルを削除」が機能しない問題を修正しました (#27, #28)。

### Build / CI
- **変更:** devcontainer から claude-code 拡張を削除しました。

### Documentation
- **追加:** GitHub Pages のダウンロードページ (`pages/index.html`) (#28)。

### Dependencies
- Vite 7.2.7, `@reduxjs/toolkit` 2.11.1, `@vitejs/plugin-react` 5.1.2, `@types/node` 24.10.2。

## [10.0.2] - 2025-12-08

### Security
- CVE-2025-66293 の修正のため libpng を 1.6.53 に更新しました。

### Dependencies
- oxipng 10, Electron 39.2.6, electron-builder 26.3.5。

## [10.0.1] - 2025-12-05

### Build / CI
- **変更:** Intel macOS のジョブで `macos-15-intel` ランナーと最新の NASM を使うようにしました。

### Documentation
- **追加:** README にプロジェクトアイコンとスクリーンショットを追加しました。

### Dependencies
- Electron 39.2.5, electron-builder 26.3.4, libpng 1.6.52, emsdk 4.0.21, React 19.2.1, Vite 7.2.6,
  Prettier 3.7.4, Alpine 3.23。

## [10.0.0] - 2025-11-26

初回公開リリース。

### Library
- `libcolopresso`: PNG を WebP (ロッシー / ロスレス)、AVIF (ロッシー / ロスレス)、最適化 PNG に変換する
  C99 ライブラリ。PNG モードはロスレス最適化 (メタデータ削除)、保護色対応の 256 色パレット減色、
  Reduced RGBA32 (自動 / 手動の目標色数、RGB / アルファのグリッドビット深度 1-8、重要度を考慮した
  ディザリング)、誤差拡散ディザと自動ディザ推定を備えた Limited RGBA4444。
- `pngx_bridge`: oxipng と libimagequant を呼び出す Rust ブリッジ。

### CLI
- OS のマルチスレッドと CPU 拡張命令を活用する Windows / Linux / macOS 向けコマンドラインコンバータ。

### GUI
- Electron デスクトップアプリ: ドロップまたは選択したフォルダ内の PNG をすべて変換、元ファイルの
  削除オプション、プロファイル対応。
- Chrome 拡張: ブラウザから直接ダウンロード、複数ファイルの ZIP アーカイブ出力 (任意)。

### JS/WASM
- Node.js 向け WebAssembly 版 CLI (`colopresso.js` / `colopresso.wasm`)。

### General
- フォーマットごとのパラメータを保存・エクスポート・インポートできるプロファイル機能。
