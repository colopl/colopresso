# セキュリティポリシー

[English](./SECURITY.md)

## サポートするバージョン

セキュリティ修正は最新のリリースにだけ行います。ライブラリ (libcolopresso)、 CLI、デスクトップアプリ (GUI)、
Python パッケージ、 Node.js (WebAssembly) 版のいずれも同じです。報告の前に、最新のリリースでも問題が起きるかを
確かめてください。

| バージョン           | サポート |
| -------------------- | -------- |
| 最新のリリース       | あり     |
| それより前のリリース | なし     |

## 脆弱性の報告

セキュリティ上の問題は、公開の Issue、 Pull Request、 Discussions には書かないでください。

GitHub の非公開の脆弱性報告 (private vulnerability reporting) で報告してください。このリポジトリの
[Security タブ](https://github.com/colopl/colopresso/security) を開き、「Report a vulnerability」を選びます。

可能な範囲で、次の情報を含めてください。

- 問題が起きる配布物 (ライブラリ、 CLI、 GUI、 Python パッケージ、 Node.js 版) とそのバージョン
- 環境 (OS とアーキテクチャ) と、 colopresso の呼び出し方
- 問題を再現できる最小の入力 (PNG ファイル) と、オプションや設定
- 起きたこと: クラッシュ、サニタイザの報告、メモリの増加、出力の誤りなど

報告を受け取ったことをお知らせし、調査の間も状況を共有します。公開の時期は報告者と相談して決めます。修正した
脆弱性は、このリポジトリの GitHub Security Advisory と [変更履歴](./CHANGELOG_ja.md) で公表します。

## 対象

colopresso は、信頼できない入手元の PNG ファイルの変換にもよく使われます。特に次のような報告を歓迎します。

- PNG ファイルや入力の組み合わせで起こせるメモリ安全性の問題
- 入力によって起きるクラッシュ、応答停止、メモリやスレッドの枯渇

colopresso が利用している第三者のライブラリ (libpng, zlib, libwebp, libavif, libaom, oxipng, libimagequant,
Electron など) の脆弱性は、それぞれのプロジェクトに報告してください。その問題が colopresso のリリースに影響する
場合は、依存関係を更新できるよう、こちらにも非公開でお知らせいただけると助かります。
