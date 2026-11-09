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
        - `/app/electron/e2e`: パッケージした Electron アプリを Playwright で操作する E2E テスト (`smoke.sh`, `driver.mjs`) です。 CI の `WindowsElectron` (x64 / arm64) と `macOSElectron` (Apple Silicon) で実行され、手順は `colopresso-electron-e2e` スキルにまとめています
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
- `/.claude/skills`: Claude Code 用のスキルです。 `rollup-dependencies` は依存関係をまとめて更新する (ロールアップ) 手順、 `colopresso-electron-e2e` はビルドした Electron アプリを Playwright で操作して GUI からの変換を確かめる手順です
- `/.devcontainer`, `Dockerfile`, `Dockerfile.alpine`: Linux 開発環境 (Dev Container) と CI 用のコンテナ定義です
    - `Dockerfile` の `toolchain` ステージ (コンパイラと SDK) は CI が Actions のキャッシュに保存して使い回します。このステージではプロジェクト全体を `COPY` せず、必要なファイルだけを読むようにしてください。読むファイルが変わるたびにキャッシュが無効になります
- `/.github/workflows`: CI (`ci.yaml`), カバレッジ (`coverage.yaml`), リリース (`release.yaml`) のワークフローです
- `CMakeLists.txt`: プロジェクト全体の CMake 構成ファイルです
- `CHANGELOG.md`, `CHANGELOG_ja.md`: 変更履歴です (英語 / 日本語)。書式は「変更履歴の記載」を参照してください

## コーディング規約
- C 言語のコードを書く時は C11 標準 (GNU 拡張なし) に準拠し、可能な限り常に標準化された型 `stdint.h` `stdbool.h` 等の標準型を用いるようにしてください
    - CMake では自前のターゲットに `colopresso_set_c_standard()` (`cmake/compiler.cmake`) を適用してください。 C11 / GNU 拡張なしを設定し、 Linux などでは `_POSIX_C_SOURCE=200809L` と `_DEFAULT_SOURCE` (`struct tm` の `tm_gmtoff` などの BSD 拡張) を定義します
    - Windows (MSVC の `/std:c11`) でもビルドするため、 MSVC が対応しない機能 (VLA, `<threads.h>`, `_Atomic` / `<stdatomic.h>`, `<complex.h>`) は使わないでください。スレッドとミューテックスは `colopresso/portable.h` の互換層を使います
- 公開 API (`library/include` のヘッダ) の型・構造体の並び・関数のシグネチャは互換性のため変えないでください
- メモリを無駄に利用しないためにも、利用する範囲が明らかに小さい整数型についてはなるべく小さい物を利用するようにしてください (例: `uint32_t` ではなく `uint8_t` 等)
- 移植性を高めるため、整数型はサイズの定まったものを積極的に利用してください (例: `uint32_t` 等)
- 可能な限り符号なし整数型を利用し、本当に必要な時以外は符号付き整数型を利用しないでください
- 画素数やバッファサイズの計算は `size_t` (または `uint64_t`) に広げてから掛け算し、掛け算の前に桁あふれを確認してください。 Emscripten (wasm32) では `size_t` が 32 bit です
- 変数宣言は全て関数の最上位で行い、型が同じでまとめて宣言できるものはまとめてください ポインタ型, 配列, 構造体であってもまとめて良いです。また特定のコードパスでしか使わないものでも定義してください (例: `uint8_t foo, *bar, baz[2];`)
    - **1 つの型は 1 つの宣言文**にします。修飾子 (`const` など) と基本型が同じものは、ポインタ・配列・初期化付きも含めて同じ文に並べます (例: `const float *m = job->matrix, knee = peak * 0.75f, range = peak - knee;`。後の宣言子は前の宣言子を初期化式で参照できます)
    - ネストしたブロック (`{ ... }`、`if` の中など) や文の後、`for (uint32_t i = 0; ...)` のような for の見出しでは宣言しません
    - c, h ファイルの並び順は以下の通りとしてください。また速度のために効果がある場所についてはインライン化を積極的に行ってください
        - `#include`
        - `#define` などマクロ
        - `static inline` 関数
        - `static` 関数
        - 通常関数 (`main`、Unity の `setUp` / `tearDown`、ライブラリ内部で共有する関数など)
        - ヘッダでエクスポートされている関数
            - 本来不要ですが、わかりやすくするために `extern` をつけてください
    - プロトタイプ宣言はなるべく使わず、使われる関数を先に定義する順序で揃えてください。 `static inline` 関数から呼ぶ (関数ポインタとして渡す) だけの補助関数やコールバックは、 `static inline` にして使う側より前に置きます
        - 上の並び順を守ると使う前に定義できない場合 (相互再帰など) に限り、プロトタイプ宣言を使ってよいです。その場合はそのグループの先頭に置きます
    - `#define` によるマクロは、可能な限りファイル上部 (`#include` の直後、最初の宣言より前) にまとめてください。関数の間や関数の中には置きません。プラットフォームや条件ごとに分ける場合も、その `#if` のブロックごと上部に置きます。例外はインクルードガードと、同じファイルの中で `#undef` する局所的な補助マクロだけです
    - 変数宣言の並び順は以下の通りとしてください
        - `const` 宣言されたもの (`const` で始まる宣言。`const T *p` も含む。並び順は以下の非 const と同一)
        - プロジェクト・依存関係固有型 (e.g. `cpres_config_t`, `png_structp`, `napi_value`)
        - `uint64_t`
        - `uint32_t`
        - `uint16_t`
        - `uint8_t`
        - `int64_t`
        - `int32_t`
        - `int16_t`
        - `int8_t`
        - `size_t`
        - `bool`
        - `char`
        - `double`
        - `float`
        - `void *`
    - `if`, `for` などは `{}` を省略せず、必ず `{}` を利用してください
    - `typedef` 等で型を定義するときは必ず suffix として `_t` を付加してください
- プログラムへのコメントは必要最小限にしてください。コードを読めばわかる処理についてコメントをする必要はありません
- C コードのフォーマットはリポジトリの `.clang-format` (LLVM ベース, 4 スペースインデント, 100 桁, Linux 式の波括弧) と `.editorconfig` に従ってください。 clang-format / clang-tidy は LLVM 23 を使います (`.clang-tidy` は静的解析の設定です)
- **処理のロジックごとに空行を入れてください**
    - 宣言の後には空行を置きます
    - `if` / `for` / `while` / `do` / `switch` のブロックの前に、同じブロック内の別の文があれば空行を置きます。例外は次の 2 つだけで、その場合は続けて書きます
        - 直前の文が、見出しで確かめる変数を代入している (`p = malloc(...);` → `if (p == NULL) {`、`&x` で受け取った出力も含む)
        - ループの直前の文が、ループ内で使う変数を準備している (`entries = ...;` → `for (...) {`)
    - ブロックの閉じ `}` の後に同じ階層の文が続くときは空行を置きます (`} else {` と、続けて閉じる `}` は除く)
    - 他の文に続く `return` / `break` / `continue` / `goto` の前には空行を置きます (ブロック内の唯一の文なら不要)
    - 種類の違う処理のまとまり (メモリ確保とその確認、代入のまとまり、副作用のある関数呼び出し、解放・後始末、エラー情報の設定) の間にも空行を置きます。同じまとまりの中には入れません
- 上の規約のうち機械的に確かめられるもの (宣言の位置・まとめ方・順序、空行、`{}`、`_t`、関数の並び、`#define` の位置、SPDX ヘッダ) は `utils/check_style.py` が検査します (`python3 -I utils/check_style.py`)。 CI で必須です。違反が出たら規約に合わせて直してください (チェッカー側を緩めるのは、規約そのものを変えるときだけ)
- ライブラリのデフォルト値は `library/include/colopresso.h` の `COLOPRESSO_*_DEFAULT_*` マクロが唯一の定義です。 CLI のヘルプ表示などアプリケーション側でデフォルト値を扱う時はこのマクロを参照し、数値を重複して記述しないでください
- TypeScript や React.js, Vite の構成を記述する時は、必ず現状のベストプラクティスを調査し、それに則った実装を行ってください

## 変更履歴の記載
- 利用者やビルド担当者に伝えるべき変更 (機能追加, 挙動やデフォルト値の変更, 不具合修正, オプションや公開 API の追加・削除, ビルド要件やツールチェーンの変更, 依存関係のメジャー更新やセキュリティ更新など) を行った時は、同じ変更の中で `CHANGELOG.md` (英語) と `CHANGELOG_ja.md` (日本語) の両方の `## [Unreleased]` 節に追記してください
- `## [Unreleased]` 節はリリース直後には存在しません。無い場合は、最新バージョンの節 (`## [X.Y.Z] - YYYY-MM-DD`) の直前に `## [Unreleased]` を新設し、その中に追記してください。既にある場合はそこへ追記します
- 記載先は変更が属するビルドタイプの節です: `Library` (libcolopresso 本体, 公開 API), `CLI`, `GUI` (Electron), `JS/WASM` (Node.js / Emscripten ビルド), `Python`, `Build / CI` (CMake, Docker, ワークフロー, Renovate, リリース処理), `Documentation`, `General`, `Security`, `Dependencies`。節名は日本語版でも英語表記のままにしてください。該当する節がなければ既存の節の並び順に合わせて新設してください
- 各項目は `- **Added:**` / `- **Changed:**` / `- **Fixed:**` / `- **Removed:**` (日本語版は `**追加:**` / `**変更:**` / `**修正:**` / `**削除:**`) で始め、末尾に PR 番号を `(#123)` の形で付けてください。定型的な依存関係の更新は `Dependencies` にバージョンだけをまとめます
- セキュリティに関わる変更 (依存関係のセキュリティ更新を含む) は、他の節より前、 `## [Unreleased]` 直下の最初の節として置いた `Security` 節に記載し、各項目の先頭に `**[SECURITY]**` を付けてください (例: `- **[SECURITY]** **Fixed:** GUI: ...`)。影響を受けるビルドタイプは項目の本文に `GUI:` のように書きます
- リリース時は `## [Unreleased]` を `## [X.Y.Z] - YYYY-MM-DD` (リリース日) に改名するだけにし、空の `## [Unreleased]` 節は残さないでください

## 注意事項
- アプリケーションのビルドを行う時は必ず `README.md` を読み、従ってください。 Dev Container を用いた Linux 環境では「Build (Linux)」、 macOS 上で直接作業する場合は「Build (macOS)」の手順を参考にしてください
    - Linux 上での Electron アプリのビルドは現状サポートしていません。 Electron アプリのビルドが必要な場合、自分でビルドしようとせず、チャットでその旨伝えてください
- Pull Request の CI は、変更が `app/`, JS の依存 (`package.json`, `pnpm-lock.yaml`, `pnpm-workspace.yaml`), ドキュメント (`*.md`, `.claude/`), `resources/`, `pages/`, `.prettierrc.json`, `.github/renovate.json` だけなら、ライブラリ・ CLI ・ WebAssembly ・ Python wheel のジョブを省きます (Electron のジョブと E2E は実行します)。省いてよい範囲を変える時は、 `.github/workflows/ci.yaml` の Prepare ジョブの判定を更新してください
- CI のジョブ名やアーティファクト名でアーキテクチャを表す時は、 macOS は `Intel` / `Apple Silicon`、 Windows は `x64` / `arm64`、 Linux は `amd64` / `arm64` と書いてください。リリース成果物のファイル名は自動更新が参照しているので、この表記に合わせて変えないでください
- macOS のリリースビルドの Runner は、最低対応 macOS が上がらないよう据え置いています (デプロイメントターゲットを指定していないため、ビルドした macOS 以降でしか動かなくなります)。 CI の Runner は Renovate が追い、リリースの macOS Runner は Renovate の対象から外しています
