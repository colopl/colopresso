# Security Policy

[日本語](./SECURITY_ja.md)

## Supported Versions

Security fixes are made for the latest release only. This applies to every distribution of
colopresso: the library (libcolopresso), the CLI, the desktop app (GUI), the Python package and the
Node.js (WebAssembly) build. Please check whether the issue still occurs with the latest release
before reporting it.

| Version        | Supported |
| -------------- | --------- |
| Latest release | Yes       |
| Older releases | No        |

## Reporting a Vulnerability

Please do not report security vulnerabilities through public GitHub issues, pull requests or
discussions.

Report them privately with GitHub's private vulnerability reporting: open the
[Security tab](https://github.com/colopl/colopresso/security) of this repository and choose
"Report a vulnerability".

Please include as much of the following as you can:

- The affected distribution (library, CLI, GUI, Python package or Node.js build) and its version
- The platform (OS and architecture) and how colopresso is called
- A minimal input (PNG file) and the options or configuration that reproduce the issue
- What you observed: a crash, a sanitizer report, memory growth, wrong output, and so on

We will confirm that we received the report, keep you informed while we investigate, and agree with
you on when the issue is disclosed. Fixed vulnerabilities are announced in a GitHub Security
Advisory of this repository and in the [changelog](./CHANGELOG.md).

## Scope

colopresso is often used to convert PNG files that come from untrusted sources. Reports about the
following are especially welcome:

- Memory safety issues that a PNG file or a combination of inputs can trigger
- Crashes, hangs and memory or thread exhaustion caused by an input

Vulnerabilities in the third-party libraries colopresso builds on (libpng, zlib, libwebp, libavif,
libaom, oxipng, libimagequant, Electron and others) should be reported to those projects. If such an
issue affects a colopresso release, you are welcome to let us know privately as well, so that we can
update the dependency.
