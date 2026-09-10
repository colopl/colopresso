## 前提事項
- チャットの回答はすべて日本語で回答してください
- プログラムソースコードにおけるコメントはすべて英語で行ってください
- 実行中のログが見えなくなってしまうので、実行しようとするコマンドの末尾に ` | tail -NUM` を付けないでください

## このプロジェクトについて
このプロジェクトは C99 で実装された画像変換・減色圧縮ライブラリである `libcolopresso` と、それを用いたアプリケーション `colopresso` (CLI / Electron GUI / Node.js WebAssembly / Python バインディング) を含んでいます。

また、 Rust で実装された外部依存ライブラリ (oxipng, libimagequant) を呼び出すための `pngx_bridge` というプロジェクトも含んでいます。

## フォルダ構成
- `/library`: `libcolopresso` ライブラリ本体です
    - `/library/pngx_bridge`: Rust 製ライブラリ (oxipng / libimagequant) を統合するためのサブプロジェクトである `pngx_bridge` のコードです。 C 向けヘッダは cbindgen で生成されます
    - `/library/include`: インストールされるヘッダファイルです。 `colopresso.h` が公開 API (`cpres_config_t` と `cpres_encode_*_memory` 等) と `COLOPRESSO_*_DEFAULT_*` デフォルト値マクロを定義します。 `*.h.in` は CMake により生成されます
    - `/library/src`: `libcolopresso` 本体のコードです
        - `colopresso.c`: 公開 API のエントリポイント (`cpres_config_init_defaults`, `cpres_encode_*_memory`, バージョン情報取得等) です
        - `avif.c`: AVIF エンコーダ本体のコードです
        - `webp.c`: WebP エンコーダ本体のコードです
        - `pngx.c`: PNGX エンコーダ本体のコードです。 `cpres_config_t` から内部オプション `pngx_options_t` への変換 (`pngx_fill_pngx_options`) もここで行います
            - `pngx_common.c`: PNGX エンコーダ共通実装 (視覚重要度マップ, 高彩度アンカー, 適応ディザ, 保護色パレット等) のコードです
            - `pngx_limited.c`: PNGX Limited RGBA4444 モードエンコーダ固有のコードです
            - `pngx_palette256.c`: PNGX 256 Palette モードエンコーダ固有のコードです (ポストプロセス平滑化, グラデーションプロファイル調整, アルファブリードを含みます)
            - `pngx_reduced.c`: PNGX Reduced RGBA32 モードエンコーダ固有のコードです
        - `png.c`: libpng による PNG デコードの共通処理です
        - `file.c`: ファイル入出力 API です。 `COLOPRESSO_WITH_FILE_OPS=ON` の時のみビルドされます
        - `emscripten.c`: Emscripten (Node.js / WebAssembly) 向けにエクスポートされる API です
        - `wasm.c`: WebAssembly 分離モード (`PNGX_BRIDGE_WASM_SEPARATION`) 用の `pngx_bridge` 呼び出し層です
        - `log.c`, `portable.c`, `simd.c`, `thread.c`, `datetime/datetime.c`: ログ, 移植性 (Windows 向け getopt / スレッド / CPU 数取得等), SIMD, 並列実行ヘルパ, 日時ユーティリティです
        - `internal/`: ライブラリ内部専用のヘッダです。 CLI などアプリケーション側からは参照しないでください
    - `/library/tests`: Unity を用いた C テストです。 `COLOPRESSO_USE_TESTS=ON` で有効化され `ctest` から実行されます
    - `/library/utils`: 開発用ユーティリティ (`benchmark`, `chkbits`, `qcheck`) です。 `COLOPRESSO_USE_UTILS=ON` で有効化されます
- `/pages`: GitHub Pages で公開用のページディレクトリです
- `/cli`: CLI 版 `colopresso` アプリケーションの実装です
    - `/cli/colopresso.c`: CLI 本体です。 オプションを追加・変更する時は `kLongOptions`, `handle_long_option` (真偽値トグルは `kPngxToggleOptions`), `print_usage`, `print_verbose_summary` を合わせて更新してください
    - `/cli/CMakeLists.txt`: CLI 版 `colopresso` アプリケーションの CMake 構成ファイルです
- `/app`: GUI 版 `colopresso` アプリケーションの実装です
    - `/app/electron`: Electron アプリケーションとしての GUI 版 `colopresso` の実装です
        - `/app/electron/native`: Electron から `libcolopresso` を呼び出す Node-API ネイティブアドオン (`colopresso_native.c`) です
    - `/app/shared`: Electron アプリケーションが利用する共通実装 (React コンポーネント, フォーマット定義 `formats/`, i18n 等) です。 CMake ビルド時に Vite で `build/electron` へバンドルされます
- `/python`: Python バインディング (`colopresso` パッケージ) です。 C 拡張 `_colopresso_ext.c` と `core.py` で構成され、 API は `python/README.md` に記載されています
- `/cmake`: CMakeLists.txt から参照される CMake の構成ファイルです
- `/utils`: リリース用更新メタデータ生成スクリプト (`*.mjs` とそのテスト), Electron 構成スクリプト, ライセンスヘッダ更新スクリプトです
- `/assets`: エンコーダーのテストに用いる画像ファイルを置いたディレクトリです
- `/resources`: アイコンやスクリーンショットなどのリソースです
- `/suppressions`: Valgrind の抑制ファイルです
- `/third_party`: サードパーティライブラリの git submodule です。このディレクトリ以下のコードは編集しないでください
- `/dist`: 何かしらのアーティファクトが生じるビルドが行われた時出力されるディレクトリです
- `/build`: ビルド用ディレクトリです。基本的に CMake で指定されます
- `/.devcontainer`, `Dockerfile`, `Dockerfile.alpine`: Linux 開発環境 (Dev Container) と CI 用のコンテナ定義です
- `/.github/workflows`: CI (`ci.yaml`), カバレッジ (`coverage.yaml`), リリース (`release.yaml`) のワークフローです
- `CMakeLists.txt`: プロジェクト全体の CMake 構成ファイルです

## コーディング規約
- C 言語のコードを書く時は C99 標準に準拠し、なるべく標準化された型 `stdint.h` `stdbool.h` 等の標準型を用いるようにしてください
- メモリを無駄に利用しないためにも、利用する範囲が明らかに小さい整数型についてはなるべく小さい物を利用するようにしてください (例: `uint32_t` ではなく `uint8_t` 等)
- 移植性を高めるため、整数型はサイズの定まったものを積極的に利用してください (例: `uint32_t` 等)。また、可能な限り符号なし整数型を利用し、本当に必要な時以外は符号付き整数型を利用しないでください
- 変数宣言はなるべく関数の最上位で行い、型が同じでまとめて宣言できるものはまとめてください ポインタ型であってもまとめて良いです (例: `uint8_t foo, *bar, baz = 0;`)
- プログラムへのコメントは必要最小限にしてください。コードを読めばわかる処理についてコメントをする必要はありません
- C コードのフォーマットはリポジトリの `.clang-format` (LLVM ベース, 2 スペースインデント, 200 桁) と `.editorconfig` に従ってください
- ライブラリのデフォルト値は `library/include/colopresso.h` の `COLOPRESSO_*_DEFAULT_*` マクロが唯一の定義です。 CLI のヘルプ表示などアプリケーション側でデフォルト値を扱う時はこのマクロを参照し、数値を重複して記述しないでください
- TypeScript や React.js, Vite の構成を記述する時は、必ず現状のベストプラクティスを調査し、それに則った実装を行ってください

## 注意事項
- アプリケーションのビルドを行う時は必ず `README.md` を読み、従ってください。 Dev Container を用いた Linux 環境では「Build (Linux)」、 macOS 上で直接作業する場合は「Build (macOS)」の手順を参考にしてください
    - Linux 上での Electron アプリのビルドは現状サポートしていません。 Electron アプリのビルドが必要な場合、自分でビルドしようとせず、チャットでその旨伝えてください
