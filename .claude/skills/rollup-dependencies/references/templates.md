# CHANGELOG と PR 本文の型

以下の版番号・PR 番号・勧告 ID はすべて仮のもの。実際の値に置き換える。

## CHANGELOG (英語版)

```markdown
## [Unreleased]

### Security
- **[SECURITY]** **Fixed:** GUI: Electron is updated to X.Y.Z (Chromium A.B.C.D). It fixes
  GHSA-xxxx-xxxx-xxxx (CVE-YYYY-NNNNN, high): <what an attacker could do, as the advisory says>.
  It also brings backported upstream security fixes from Chromium, V8 and others (#NNN).
- **[SECURITY]** **Fixed:** Library: <library> is updated from A to B, which fixes <class of issue>
  (CVE-YYYY-NNNNN and others) (#NNN).
- **[SECURITY]** **Fixed:** Build / CI: vulnerable transitive build-tool dependencies (<names>) are
  updated to patched releases. <name> still has an advisory (GHSA-...) because <reason> (#NNN).

### GUI
- **Changed:** The GUI runs on Electron N (Chromium M, Node.js K) instead of Electron N-1 (#NNN).

### Build / CI
- **Changed:** <build change that follows an upstream change> (#NNN).

### Dependencies
- Electron X.Y.Z, Vite X.Y.Z, pnpm X.Y.Z, emsdk X.Y.Z, Rust X.Y.Z, ...
```

日本語版 (`CHANGELOG_ja.md`) は同じ節の構成で、`**修正:**` / `**変更:**` を使う。節名は英語のまま。

## PR 本文 (日本語)

```markdown
open 中の Renovate PR N 件と、Dependency Dashboard で保留されている更新をまとめて取り込みます。
あわせて、Renovate が追っていない依存 (<種類>) のセキュリティ修正を取り込みます。

## 取り込む更新

| 出所 | 対象 | 変更 | 備考 |
| --- | --- | --- | --- |
| #NNN | electron | A → **B** | Renovate の提案は C。<なぜ B にしたか> |
| Dashboard | <pkg> | A → B | |
| 独自 | <pkg> | A → B | <Renovate が追わない理由> |

lockfile だけを更新する JS の PR は、pnpm で lockfile を作り直して取り込んでいます
(`package.json` の範囲指定は変えていません)。

## 取り込まない更新
- <対象>: <理由>

## セキュリティ
- **<対象> X.Y.Z**: <勧告 ID と内容、このプロジェクトのどこに効くか>
- 残り: <残っている勧告と理由>

## 追従した変更
- <上流の変更に合わせてビルド構成などを直した内容>

## 後で外すもの
- <待機期間の除外、上流が追いついたら不要になる上書き>

## 確認したこと
- [x] `pnpm install --frozen-lockfile` (supply-chain チェックが通る)、`pnpm audit` の残り
- [x] 型検査 (`tsc --noEmit`)、`prettier --check`、`node --test utils/*.test.mjs`
- [x] macOS arm64: ネイティブ Release ビルドと ctest (N/N)
- [x] macOS arm64: Electron アプリのビルドとパッケージング (dmg/zip、Electron X.Y.Z)
- [x] Linux arm64 の CI コンテナ: ネイティブ / ASan / UBSan / 統合 Node/WASM のビルドと ctest
- [ ] CI: Windows / macOS x64 / Linux amd64 / Python wheel

Closes #NNN
```
