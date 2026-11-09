# 依存の定義場所と調べ方

棚卸しのときに上から確認する。「Renovate」の列が「追う」でも、Dashboard で保留されていたり、
`latest` 以外のタグに新しい版があったりするので、候補の版は自分でも確かめる。

## 目次

- npm (直接依存 / 推移的依存 / Electron / pnpm)
- Rust (ツールチェーン / wasm-pack / crate)
- git submodule の C ライブラリと emsdk
- libavif が取得する libaom
- Docker と CI のツール
- GitHub Actions
- Python と Node.js

## npm

| 対象 | 定義場所 | Renovate | 調べ方 |
|---|---|---|---|
| 直接依存 | `package.json`, `pnpm-lock.yaml` | 追う | `pnpm outdated --long`、`npm view <pkg> time --json` (公開日時)、`npm view <pkg> dist-tags` |
| 推移的依存 | `pnpm-lock.yaml` | 追わない (lockfile maintenance なし) | `pnpm audit --json`、`pnpm why <pkg>`。修正版が既存の範囲に収まるか確かめる |
| Electron | `package.json` の `electron` | 追う | 下記 |
| pnpm 本体 | `package.json` の `packageManager` | 追う | `gh api repos/pnpm/pnpm/releases` の本文の `#### Security` 節 |

### Electron

- サポートされるのは最新の 3 メジャー。`npm view electron dist-tags` で各系列の最新を確かめる。
- リリースノート: `gh api repos/electron/electron/releases/tags/v<版> --jq .body`。
  「Backported fixes from upstream ...」がセキュリティ修正のバックポート。`[!NOTE]` に勧告が書かれることもある。
- 勧告: `gh api 'repos/electron/electron/security-advisories?per_page=20&sort=published&direction=desc'`。
  現在の版より後の修正版を持つものを拾う。アプリは electron-updater で更新サーバーと TLS 通信するので、
  通信まわりの勧告は該当しうる。
- メジャー更新では「Breaking Changes」を読み、`app/electron/main.ts` と `preload.ts` で使っている API と
  突き合わせる。preload はサンドボックスで動く (`nodeIntegration: false`, `contextIsolation: true`)。
- Electron が内蔵する Node.js のメジャーは、CI の `node-version` と `@types/node` に揃える。

### electron-builder / electron-updater

`latest` タグより新しい patch が `v26` のような系列タグで出ることがある。electron-updater はアプリに
同梱されるので、更新ファイルの検証やリダイレクトの扱いの修正はセキュリティ関連として扱う。

## Rust

| 対象 | 定義場所 | Renovate | 調べ方 |
|---|---|---|---|
| ツールチェーン | `rust-toolchain.toml` と `library/pngx_bridge/rust-toolchain.toml` (両方) | 追う | Renovate の PR |
| wasm-pack | `rust-toolchain.toml` の `[colopresso]` | 追う (custom manager) | `gh api repos/wasm-bindgen/wasm-pack/releases` |
| crate | `library/pngx_bridge/Cargo.toml`, `Cargo.lock` | 直接依存は追う | `cargo update --dry-run`、`scripts/cargo_lock_report.py` |

`Cargo.toml` の範囲は広め (例: `wasm-bindgen = "0.2.118"`) なので、更新は `Cargo.lock` だけで済むことが多い。
crates.io の API を叩くときは汎用の User-Agent を付ける (メールアドレスは入れない)。

## git submodule

`.gitmodules` の `branch` にタグ名、gitlink がそのタグのコミット。更新は次の 3 つを揃える。

```bash
git -C third_party/<name> fetch --depth 1 origin tag <tag>
git -C third_party/<name> checkout <tag>
git config -f .gitmodules submodule.third_party/<name>.branch <tag>
```

| 対象 | 気をつけること |
|---|---|
| emsdk | emscripten の `ChangeLog.md` で、削除された設定や API を使っていないか確かめる。Docker イメージは `EMSDK_VERSION` にこのタグを渡す |
| zlib | CMake のターゲット名・出力名の変更に注意。`cmake/thirdparty.cmake` は Windows で静的ライブラリのパスを固定している |
| libpng | 1.6 系を追う。1.7 のベータ版のタグは対象外 |
| libwebp, libavif, Unity | リリースタグを追う |

上流の CMake の変更 (最低バージョン、オプション名、ライブラリ名) は、リリースノートと
`git diff <旧タグ> <新タグ> -- CMakeLists.txt` で確かめる。

## libaom (libavif が取得する)

`AVIF_CODEC_AOM=LOCAL` なので、libavif は設定時に libaom を FetchContent で取得する。取得する版は
`third_party/libavif/cmake/Modules/LocalAom.cmake` の `AVIF_AOM_GIT_TAG`。

- 最新のタグ: `git ls-remote --tags https://aomedia.googlesource.com/aom`
- 変更点: `curl -s "https://aomedia.googlesource.com/aom/+/refs/tags/<tag>/CHANGELOG?format=TEXT" | base64 -d`
- libavif の main が上げているか: `gh api "repos/AOMediaCodec/libavif/commits?path=cmake/Modules/LocalAom.cmake&per_page=5"`

libavif のリリースより新しい libaom にセキュリティ修正があるときは、`cmake/thirdparty.cmake` の
`COLOPRESSO_AOM_VERSION` を上げる。libavif の submodule がその版以上を固定すれば上書きは自動的に
無効になるので、そのときは上書きのコードごと削除する。ビルドログに
`libaom: overriding libavif's ...` が出ているか、`_deps/libaom-src` のタグで確かめる。

## Docker と CI のツール

| 対象 | 定義場所 | Renovate | 調べ方 |
|---|---|---|---|
| Debian / Alpine | `Dockerfile` の `DEBIAN_VERSION`, `Dockerfile.alpine` の `ALPINE_VERSION` | 追う | Dashboard |
| LLVM (clang, sanitizer) | `Dockerfile` の apt.llvm.org のメジャー番号 | 追わない | ツールチェーンの変更なので、依存更新とは分けてユーザーに相談する |
| Valgrind | `Dockerfile` の `VALGRIND_VERSION` | 追う (custom datasource) | `https://sourceware.org/pub/valgrind/` の一覧 |

## GitHub Actions

| 固定のしかた | 例 | 調べ方 |
|---|---|---|
| メジャータグ | `actions/checkout@v7` | 何もしなくてよい |
| SHA + バージョンコメント | `pnpm/action-setup@<sha> # v6.1.0` | `gh api repos/<owner>/<repo>/commits/<tag> --jq .sha` でタグのコミットを引いて置き換える |
| SHA のみ | `dtolnay/rust-toolchain@<sha>` | `gh api repos/<owner>/<repo>/compare/<sha>...master` で差分のコミットを読み、修正があれば上げる |

`ci.yaml` と `release.yaml` の両方を置き換える。

## Python と Node.js

- cibuildwheel: `python/requirements.txt` の範囲 (`>=4.1,<5`) で CI が最新を入れる。
- Python: ワークフローの `python-version`。配布する wheel は `cp310-abi3`。
- Node.js: ワークフローの `node-version`。Electron が内蔵する Node のメジャーに揃える
  (`curl -s https://nodejs.org/dist/index.json` で LTS の状態も確かめられる)。
