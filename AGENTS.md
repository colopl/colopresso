## 前提事項
- チャットの回答はすべて日本語で回答してください
- プログラムソースコードにおけるコメントはすべて英語で行ってください

## このプロジェクトについて

このプロジェクトは C99 で実装された画像変換・減色圧縮ライブラリである `libcolopresso` と、それを用いたアプリケーション `colopresso` 含んでいます。
また、 Rust で実装された外部依存ライブラリを呼び出すための `pngx_bridge` というプロジェクトも含んでいます。

## フォルダ構成

- `/library`: `libcolopresso` ライブラリ本体です
    - `/library/pngx_bridge`: Rust 製ライブラリを統合するためのグループロジェクトである `pngx_bridge` のコードです
    - `/library/include`: インストールされるヘッダファイルです
    - `/library/src`: `libcolopresso` 本体のコードです
        - `avif.c`: AVIF エンコーダ本体のコードです
        - `pngx.c`: PNGX エンコーダ本体のコードです
            - `pngx_common.c`: PNGX エンコーダ共通実装のコードです
            - `pngx_limited4444.png` PNGX Limited4444 モードエンコーダ固有のコードです
            - `pngx_palette256.png` PNGX 256 Palette モードエンコーダ固有のコードです
            - `pngx_reduced.c` PNGX Reduced RGBA32 モードエンコーダ固有のコードです
        - `webp.c`: WebP エンコーダ本体のコードです
- `/pages`: GitHub Pages で公開用のページディレクトリです
- `/cli`: CLI 版 `colopresso` アプリケーションの実装です
    - `/cli/CMakeLists.txt`: CLI 版 `colopresso` アプリケーションの CMake 構成ファイルです
- `/app`: GUI 版 `colopresso` アプリケーションの実装です
    - `/app/chrome` Chrome Extension としての GUI 版 `colopresso` の実装です
    - `/app/electron` Electron アプリケーションとしての GUI 版 `colopresso` の実装です
    - `/app/shared`: Chrome Extension / Electron 共通の実装です。 CMake によりビルド時に自動的にコピーされます
- `/cmake`: CMakeLists.txt から参照される CMake の構成ファイルです
- `/assets`: エンコーダーのテストに用いる画像ファイルを置いたディレクトリです
- `/third_party`: サードパーティライブラリの git submodule です。このディレクトリ以下のコードは編集しないでください
- `/dist`: 何かしらのアーティファクトが生じるビルドが行われた時出力されるディレクトリです
- `/build`: ビルド用ディレクトリです。基本的に CMake で指定されます
- `CMakeLists.txt`: プロジェクト全体の CMake 構成ファイルです

## コーディング規約
- C 言語のコードを書く時は C99 標準に準拠し、なるべく標準化された型 `stdint.h` `stdbool.h` 等の標準型を用いるようにしてください
- メモリを無駄に利用しないためにも、利用する範囲が明らかに小さい整数型についてはなるべく小さい物を利用するようにしてください (例: `uint32_t` ではなく `uint8_t` 等)
- 移植性を高めるため、整数型はサイズの定まったものを積極的に利用してください (例: `uint32_t` 等)。また、可能な限り符号なし整数型を利用し、本当に必要な時以外は符号付き整数型を利用しないでください
- 変数宣言はなるべく関数の最上位で行い、型が同じでまとめて宣言できるものはまとめてください ポインタ型であってもまとめて良いです (例: `uint8_t foo, *bar, baz = 0;`)
- プログラムへのコメントは必要最小限にしてください。コードを読めばわかる処理についてコメントをする必要はありません
- TypeScript や React.js, Vite の構成を記述する時は、必ず現状のベストプラクティスを調査し、それに則った実装を行ってください

## 注意事項
- アプリケーションのビルドを行う時は必ず `README.md` を読み、従ってください。開発環境は Linux 上であるため Linux 用でのビルド方法を参考にしてください
    - Linux 上での Electron アプリのビルドは現状サポートしていません。 Electron アプリのビルドが必要な場合、自分でビルドしようとせず、チャットでその旨伝えてください
