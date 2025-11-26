# libcolopresso / colopresso

PNG 画像を新しいフォーマットに変換または最適化するライブラリ (兼 アプリケーション)

<p align="center">
  <img src="resources/icon.png" alt="colopresso icon" />
</p>
<p align="center">
  <img src="resources/screenshot_ja.png" alt="colopresso screenshot (japanese)" width="50%" />
</p>

## 目次

- [概要](#概要)
- [種類](#種類)
- [開発環境](#開発環境)
- [ビルドオプション](#ビルドオプション)
- [ビルド (Linux)](#ビルド-linux)
- [ビルド (macOS)](#ビルド-macos)
- [ビルド (Node.js)](#ビルド-nodejs)
- [ビルド (Chrome Extension)](#ビルド-chrome-extension)
- [ビルド (Electron)](#ビルド-electron)
- [ライセンス](#ライセンス)
- [特記事項](#特記事項)

## 種類

### CLI

- 各OS上でコンソールアプリケーションとして動作します
- パフォーマンスのためにOSレベルのマルチスレッドとCPU拡張命令を最大限に活用します
- Windows, Linux, macOS で利用可能です

### Chrome Extension

- Google Chrome 拡張機能として動作します
- 変換されたファイルは Chrome によって直接ダウンロードされます
- フォルダや複数の PNG ファイルを一度にドロップし、変換されたファイルを ZIP アーカイブとしてダウンロードする機能をサポートしています（設定で変更可能）

> [!NOTE]
> 公式の Chrome 拡張機能は現在 Chrome ウェブストアで公開されておらず、今後リリースの予定もありません。

### Electron

- デスクトップアプリケーションとして動作します
- 選択またはドロップされたフォルダに含まれるすべての PNG ファイルを変換して保存します
    - オプションで、変換後に元の PNG ファイルを削除することも可能です

### Node.js (with WebAssembly)

- Node.js 18 以降が利用可能な場合、Emscripten でビルドされた CLI (`colopresso.js`) をそのまま実行できます
- `COLOPRESSO_NODE_BUILD=ON` と `COLOPRESSO_USE_CLI=ON` の両方が設定されている場合、`./build/cli/colopresso.js` と `colopresso.wasm` が生成され、`node ./build/cli/colopresso.js --help` などのコマンドが実行可能になります

### 共通機能

- サポートされる出力形式
    - WebP (圧縮 / ロスレス)
    - AVIF (圧縮 / ロスレス)
    - PNG (256色パレット, Reduced RGBA32, Limited RGBA4444, ロスレス)
        - ロスレス圧縮 (メタデータの削除)
        - 256色量子化 (保護色指定あり)
        - Reduced RGBA32 量子化 (自動または手動の色ターゲット、8-bit RGBA 出力を維持)
            - CLI (`--reduce-bits-rgb`, `--reduce-alpha`) または GUI の詳細パネルで RGB / アルファグリッドのビット深度 (1-8, デフォルト 4) を制御可能。重要度を考慮したディザリング付き
            - 反復的な改善を行う重要度重み付きクラスタリングにより、PSNR を維持しつつ、より小さな RGBA PNG を生成します
        - Limited RGBA4444 ビット深度量子化 (誤差拡散ディザリングと自動ディザヒューリスティック付き)
            - `--dither -1` を設定するか、GUI フィールドを空欄にすることで自動推定を行います

- プロファイル機能
    - フォーマットごとのパラメータを微調整し、プロファイルを保存、エクスポート、またはインポートできます

#### フォーマットのガイダンス

- サポートする最小デバイスが iOS 16 以降の場合、一般的に最高の画質を提供する AVIF を使用できます。
- サポートする最小デバイスが iOS 14 以降の場合、WebP を使用してください。
- サポートする最小デバイスが iOS 14 より前の場合、PNG を使用してください。

> [!NOTE]
> Android 5.x 以降は常に Google Chrome 経由の Chromium に依存しているため、追加の考慮事項は不要です（すべての形式がサポートされています）。

## 開発環境

```bash
$ git clone --recursive "https://github.com/colopl/colopresso.git"
```

クローンした `libcolopresso` ディレクトリを Visual Studio Code で開き、`Dev Containers` 拡張機能を使用して Dev Container にアタッチしてください。

> [!NOTE]
> SIMD (NEON) を有効にした arm64 上で MemorySanitizer を実行すると、`libpng` が未初期化メモリの読み取りとしてフラグ付けされる場合があります。現在、MemorySanitizer の実行中は SIMD (NEON) を無効にしています。

> [!NOTE]
> i386 / amd64 上で MemorySanitizer を使用しながらアセンブリを有効にすると、`libaom` で誤検知が発生する場合があります。現在、MemorySanitizer の実行中はアセンブラコードを無効にしています。

> [!NOTE]
> マルチスレッドを有効にすると、Rayon の設計により Valgrind / MemorySanitizer が未初期化メモリアクセスやリソースリークを報告する場合があります。これを軽減するための抑制ファイル (suppression files) が提供されています。

## ビルドオプション

### 常に利用可能

- `COLOPRESSO_USE_CLI`        ON/OFF [デフォルト: OFF] CLI バイナリのビルドを有効にします。`COLOPRESSO_WITH_FILE_OPS=ON` が必要です。
- `COLOPRESSO_USE_UTILS`      ON/OFF [デフォルト: OFF] `library/utils/` 以下のコードをビルドします。`COLOPRESSO_WITH_FILE_OPS=OFF` の場合、自動的に無効になります。
- `COLOPRESSO_USE_TESTS`      ON/OFF [デフォルト: OFF] `library/tests/` 以下のコードをビルドします。
- `COLOPRESSO_WITH_FILE_OPS`  ON/OFF [デフォルト: ON] ファイル I/O API (`cpres_encode_*_file`, `cpres_png_decode_from_file`) を有効にします。標準ファイル I/O (`fopen`, `fwrite` など) に依存します。メモリベースの API (`cpres_encode_*_memory`) は常に利用可能です。Chrome Extension または Electron ビルドを有効にすると、強制的に `OFF` になります。

### GCC (GNU C Compiler) && `-DCMAKE_BUILD_TYPE=Debug`
- `COLOPRESSO_USE_VALGRIND`     ON/OFF 利用可能な場合、Valgrind 統合を有効にします。
- `COLOPRESSO_USE_COVERAGE`     ON/OFF 利用可能な場合、`gcov` カバレッジを有効にします。

### Clang && `-DCMAKE_BUILD_TYPE=Debug`
- `COLOPRESSO_USE_ASAN`         ON/OFF 利用可能な場合、AddressSanitizer を有効にします。
- `COLOPRESSO_USE_MSAN`         ON/OFF 利用可能な場合、MemorySanitizer を有効にします。
- `COLOPRESSO_USE_UBSAN`        ON/OFF 利用可能な場合、UndefinedBehaviorSanitizer を有効にします。

## ビルド (Linux)

1. VS Code と Docker (または互換ソフトウェア) をインストールし、リポジトリディレクトリを開きます
2. Dev Containers を使用してアタッチします
3. `rm -rf "build" && cmake -B "build" -DCMAKE_BUILD_TYPE=Release -DCOLOPRESSO_USE_UTILS=ON -DCOLOPRESSO_USE_TESTS=ON -DCOLOPRESSO_USE_CLI=ON`
4. `cmake --build "build" --parallel`
5. `ctest --test-dir "build" --output-on-failure --parallel`
6. `./build/cli/colopresso` に CLI バイナリが、`./build/utils/cpres` にユーティリティバイナリが含まれ、`./build` に `libcolopresso.a` が含まれます

### カバレッジ出力

1. `rm -rf "build" && cmake -B "build" -DCMAKE_BUILD_TYPE=Debug -DCOLOPRESSO_USE_COVERAGE=ON -DCOLOPRESSO_USE_TESTS=ON`
2. `cmake --build "build" --parallel`
3. `cmake --build "build" --target coverage`
4. カバレッジレポートは `./build/coverage/html/` 以下に生成されます

### Valgrind チェック

1. `rm -rf "build" && cmake -B "build" -DCMAKE_BUILD_TYPE=Debug -DCOLOPRESSO_USE_VALGRIND=ON -DCOLOPRESSO_USE_TESTS=ON`
2. `cmake --build "build" --parallel`
3. `ctest --test-dir "build" --output-on-failure --parallel`

### Sanitizer チェック

1. `rm -rf "build" && cmake -B "build" -DCMAKE_C_COMPILER="$(which "clang")" -DCMAKE_CXX_COMPILER="$(which "clang++")" -DCMAKE_BUILD_TYPE=Debug -DCOLOPRESSO_USE_(ASAN|MSAN|UBSAN)=ON -DCOLOPRESSO_USE_TESTS=ON`
2. `cmake --build "build" --parallel`
3. `ctest --test-dir "build" --output-on-failure --parallel`

## ビルド (macOS)

1. Homebrew 経由で `cmake` をインストールします (Intel CPU の場合は `nasm` も)
2. ターミナルでクローンディレクトリに移動します
3. `rm -rf "build" && cmake -B "build" -DCMAKE_BUILD_TYPE=Release -DCOLOPRESSO_USE_UTILS=ON -DCOLOPRESSO_USE_TESTS=ON -DCOLOPRESSO_USE_CLI=ON`
4. `cmake --build "build" --parallel`
5. `ctest --test-dir "build" --output-on-failure --parallel`
6. `./build/cli/colopresso` と `./build/utils/cpres` に実行可能ファイルが含まれ、`./build` に `libcolopresso.a` が含まれます

## ビルド (Node.js)

1. VS Code と Docker (または互換ソフトウェア) をインストールし、リポジトリディレクトリを開きます
2. Dev Containers を使用してアタッチします
3. `rm -rf "build" && emcmake cmake -B "build" -DCMAKE_BUILD_TYPE=Release -DCOLOPRESSO_USE_UTILS=ON -DCOLOPRESSO_USE_TESTS=ON -DCOLOPRESSO_USE_CLI=ON -DCOLOPRESSO_NODE_BUILD=ON`
4. `cmake --build "build" --parallel`
5. `ctest --test-dir "build" --output-on-failure --parallel`
6. `COLOPRESSO_USE_UTILS=ON` の場合、Node.js 対応の `*.js` / `*.wasm` ファイルが `./build/utils/` 以下に配置されます。`COLOPRESSO_USE_CLI=ON` の場合、`./build/cli/colopresso.js` / `colopresso.wasm` が `libcolopresso.a` と共に生成されます

## ビルド (Chrome Extension)

1. VS Code と Docker (または互換ソフトウェア) をインストールし、リポジトリディレクトリを開きます
2. Dev Containers を使用してアタッチします
3. `rm -rf "build" && emcmake cmake -B "build" -DCMAKE_BUILD_TYPE=Release -DCOLOPRESSO_CHROME_EXTENSION=ON`
    - ファイル I/O API は自動的に無効になり、メモリベースの API のみが利用可能になります
4. `cmake --build "build" --parallel`
5. 拡張機能のビルド成果物は `./build/chrome` 以下に配置されます

## ビルド (Electron)

### 共通要件

- Node.js
- `wasm32-unknown-emscripten` ターゲットを持つ Rust stable
- `third_party/emsdk` と同じタグでインストールされた EMSDK
- `PATH` 経由でアクセス可能な `emcmake` / `cmake`
- Electron ビルドではファイル I/O API が自動的に無効になり、メモリ API のみが利用可能になります

### macOS

> [!TIP]
> 信頼できる最新の手順が必要な場合は、常に `release.yaml` を参照してください。

1. `third_party/emsdk` で `./emsdk install <tag>` / `./emsdk activate <tag>` を実行し、`. ./emsdk_env.sh` を source します
2. `rm -rf "build" && emcmake cmake -B "build" -DCOLOPRESSO_ELECTRON_APP=ON -DCOLOPRESSO_ELECTRON_TARGETS="--mac"`
3. `cmake --build "build" --config Release --parallel`
4. 成果物は `dist_build/colopresso-<version>_{x64,arm64}.dmg` に書き込まれます

### Windows

> [!TIP]
> cmd (コマンドプロンプト) ではなく、常に PowerShell を使用してください。

1. `third_party/emsdk\emsdk.ps1 install <tag>` / `activate <tag>` を実行し、`emsdk_env.ps1` を適用します
1. `rm -rf "build" && emcmake cmake -B "build" -DCOLOPRESSO_ELECTRON_APP=ON -DCOLOPRESSO_ELECTRON_TARGETS="--win"`
1. `cmake --build "build" --config Release --parallel`
1. 成果物は `dist_build/colopresso-<version>_{ia32,x64,arm64}.exe` として表示されます

## ライセンス

GNU General Public Licence 3 (GPLv3)

詳細は [`LICENSE`](./LICENSE) を参照してください。

## 作者

- Go Kudo <g-kudo@colopl.co.jp>

- Icon resource designed by Moana Sato

## 特記事項

[`NOTICE`](./NOTICE) を参照してください。

このソフトウェアは、大規模言語モデル (LLM) の支援を受けて開発されました。
すべての設計上の決定は人間によって行われ、LLM によって生成されたすべてのコードは、正確性とコンプライアンスについてレビューおよび検証されています。

このソフトウェアのアイコンおよびグラフィックアセットは、生成 AI を使用せずに作成されました。
`assets/` ディレクトリ内のファイルは、Python コードからプログラム的に生成されたテスト用画像です。

使用されたコーディングエージェント:

- GitHub, Inc. / GitHub Copilot
- Anthropic PBC / Claude Code
- Anysphere, Inc. / Cursor
