ARG PLATFORM=${BUILDPLATFORM:-linux/amd64}
ARG DEBIAN_VERSION=trixie-20260406
ARG ENABLE_VALGRIND=1
ARG ENABLE_CLANG=1
ARG ENABLE_EMSDK=1
ARG EMSDK_VERSION=0

FROM --platform=${PLATFORM} debian:${DEBIAN_VERSION} AS builder

# Timezone
ENV TZ="Asia/Tokyo"

# apt-get optimization
ENV DEBIAN_FRONTEND="noninteractive"
RUN rm -f /etc/apt/apt.conf.d/docker-clean; \
    echo 'Binary::apt::APT::Keep-Downloaded-Packages "true";' > "/etc/apt/apt.conf.d/keep-cache"

# Dependencies
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    set -e; \
    apt-get update && \
    apt-get --no-install-recommends install -y \
      "ca-certificates" "tzdata" "curl" \
      "bash" \
      "git" \
      "xz-utils" \
      "gcc" "g++" \
      "make" "cmake" \
      "nasm" \
      "libc6-dev" "libc6-dbg" "libstdc++-14-dev" \
      "pkg-config" \
      "python3" \
      "lcov"

SHELL [ "/bin/bash", "-c" ]
CMD [ "/bin/bash" ]

COPY rust-toolchain.toml /tmp/rust-toolchain.toml
RUN set -e; \
    RUST_STABLE_VERSION="$(grep -E '^channel\s*=' /tmp/rust-toolchain.toml | sed 's/.*=\s*"\([^"]*\)".*/\1/')"; \
    RUST_NIGHTLY_VERSION="$(grep -E '^nightly\s*=' /tmp/rust-toolchain.toml | sed 's/.*=\s*"\([^"]*\)".*/\1/')"; \
    WASM_PACK_VERSION="$(grep -E '^wasm-pack\s*=' /tmp/rust-toolchain.toml | sed 's/.*=\s*"\([^"]*\)".*/\1/')"; \
    echo "RUST_STABLE_VERSION=${RUST_STABLE_VERSION}" > /tmp/rust-versions.env; \
    echo "RUST_NIGHTLY_VERSION=${RUST_NIGHTLY_VERSION}" >> /tmp/rust-versions.env; \
    echo "WASM_PACK_VERSION=${WASM_PACK_VERSION}" >> /tmp/rust-versions.env; \
    rm "/tmp/rust-toolchain.toml"

ENV RUSTUP_HOME="/opt/rust/rustup"
ENV CARGO_HOME="/opt/rust/cargo"
RUN --mount=type=cache,target=/opt/rust/cargo/registry,sharing=locked \
    --mount=type=cache,target=/opt/rust/cargo/git,sharing=locked \
    set -e; \
    . /tmp/rust-versions.env; \
    curl --proto '=https' --tlsv1.2 -sSf "https://sh.rustup.rs" | sh -s -- -y --default-toolchain "${RUST_STABLE_VERSION}" --profile "minimal" --no-modify-path && \
    echo 'export PATH="${CARGO_HOME}/bin:${PATH}"' > "/etc/profile.d/cargo.sh" && \
    . "/etc/profile.d/cargo.sh" && \
    rustup toolchain install "${RUST_STABLE_VERSION}" && \
    rustup target add "wasm32-unknown-emscripten" && \
    rustup target add "wasm32-unknown-unknown" && \
    rustup toolchain install "${RUST_NIGHTLY_VERSION}" && \
    rustup component add "rust-src" --toolchain "${RUST_NIGHTLY_VERSION}" && \
    rustup target add "wasm32-unknown-emscripten" --toolchain "${RUST_NIGHTLY_VERSION}" && \
    rustup target add "wasm32-unknown-unknown" --toolchain "${RUST_NIGHTLY_VERSION}" && \
    cargo install "wasm-pack" --version "${WASM_PACK_VERSION}" --locked

ARG ENABLE_CLANG
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    set -e; \
    if test "${ENABLE_CLANG}" != "0"; then \
      apt-get update && \
      apt-get --no-install-recommends install -y "gnupg" && \
      LLVM_APT_CODENAME="$(. "/etc/os-release" && printf '%s' "${VERSION_CODENAME}")" && \
      test -n "${LLVM_APT_CODENAME}" && \
      mkdir -p "/usr/share/keyrings" && \
      curl -fsSL "https://apt.llvm.org/llvm-snapshot.gpg.key" | gpg --dearmor --yes -o "/usr/share/keyrings/llvm-archive-keyring.gpg" && \
      echo "deb [signed-by=/usr/share/keyrings/llvm-archive-keyring.gpg] https://apt.llvm.org/${LLVM_APT_CODENAME}/ llvm-toolchain-${LLVM_APT_CODENAME}-22 main" > "/etc/apt/sources.list.d/llvm.list" && \
      apt-get update && \
      apt-get install --no-install-recommends -y \
        "clang-22" "clang-tools-22" "clang-format-22" "clang-tidy-22" \
        "libclang-rt-22-dev" "lld-22" "lldb-22" \
        "libc++-22-dev" "libc++abi-22-dev" \
        "llvm-22" "llvm-22-dev" "llvm-22-runtime" && \
      update-alternatives --install "/usr/bin/clang" clang "/usr/bin/clang-22" 100 && \
      update-alternatives --install "/usr/bin/clang++" clang++ "/usr/bin/clang++-22" 100 && \
      update-alternatives --install "/usr/bin/clang-format" clang-format "/usr/bin/clang-format-22" 100 && \
      update-alternatives --install "/usr/bin/clang-tidy" clang-tidy "/usr/bin/clang-tidy-22" 100 && \
      update-alternatives --install "/usr/bin/lldb" lldb "/usr/bin/lldb-22" 100 && \
      update-alternatives --install "/usr/bin/ld.lld" ld.lld "/usr/bin/ld.lld-22" 100 && \
      update-alternatives --install "/usr/bin/llvm-symbolizer" llvm-symbolizer "/usr/bin/llvm-symbolizer-22" 100 && \
      update-alternatives --install "/usr/bin/llvm-config" llvm-config "/usr/bin/llvm-config-22" 100; \
    fi

# Copy project files
COPY . "/project"

# Valgrind
ARG ENABLE_VALGRIND
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    set -e; \
    if test "${ENABLE_VALGRIND}" != "0"; then \
      apt-get update && \
      apt-get --no-install-recommends install -y \
        "build-essential" "autotools-dev" "automake" "autoconf" "libtool" \
        "libc6-dev" "linux-libc-dev" \
        "libxml2-dev" && \
      cd "/project/third_party/valgrind" && \
      ./autogen.sh && \
      ./configure && \
      make -j"$(nproc)" && \
      make install && \
      cd -; \
    fi

# Emscripten SDK
ARG ENABLE_EMSDK
ARG EMSDK_VERSION
RUN set -e; \
    if test "${ENABLE_EMSDK}" != "0"; then \
      cd "/project/third_party/emsdk" && \
        if test "${EMSDK_VERSION}" = "0"; then \
          EM_GIT_TAG="$(git describe --tags --abbrev=0)"; \
        else \
          EM_GIT_TAG="${EMSDK_VERSION}"; \
        fi && \
        ./emsdk install "${EM_GIT_TAG}" && \
        ./emsdk activate "${EM_GIT_TAG}" && \
        echo ". \"$(pwd)/emsdk_env.sh\"" >> "/etc/profile.d/emsdk.sh" && \
      cd -; \
    fi

ENV EMSDK_QUIET=1

# pnpm
RUN curl -fsSL "https://get.pnpm.io/install.sh" | /bin/bash - && \
    echo 'export PATH="${HOME}/.local/share/pnpm:${PATH}"' > "/etc/profile.d/pnpm.sh"

WORKDIR "/project"

# Development container
FROM --platform=${PLATFORM} builder AS devcontainer

ENV DEBIAN_FRONTEND=""

ENV PIP_BREAK_SYSTEM_PACKAGES=1
ENV PIP_ROOT_USER_ACTION="ignore"

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    set -e; \
    apt-get update && \
    apt-get --no-install-recommends install -y \
      "ssh" "gdb" "bash" \
      "imagemagick" "python3" "python3-dev" "python3-pip" "python3-venv" "python3-build" \
      "vim" "bsdmainutils" "strace" "pngtools" "ripgrep" "exiftool" && \
    echo 'export PATH="${HOME}/.local/share/pnpm:${PATH}"' >> "/etc/bash.bashrc" && \
    update-alternatives --install "/usr/bin/python" python "$(command -v "python3")" 100 && \
    update-alternatives --install "/usr/bin/pip" pip "$(command -v "pip3")" 100 && \
    pip install --resume-retries=5 --no-cache-dir "numpy" "opencv-python" \
      "Pillow" "scikit-image" "scikit-build-core" "flake8" "black" && \
    . "/etc/profile.d/cargo.sh" && \
    rm -f "/tmp/rust-versions.env"

ENV EDITOR="vim"
ENV VISUAL="vim"

COPY .devcontainer/resources/.bashrc /root/.bashrc
COPY .devcontainer/resources/bin/* /usr/local/bin/
