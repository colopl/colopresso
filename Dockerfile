ARG PLATFORM=${BUILDPLATFORM:-linux/amd64}
ARG DEBIAN_VERSION=trixie
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
      "ca-certificates" "tzdata" "lsb-release" "curl" \
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
    rm /tmp/rust-toolchain.toml

ENV RUSTUP_HOME="/opt/rust/rustup"
ENV CARGO_HOME="/opt/rust/cargo"
RUN --mount=type=cache,target=/opt/rust/cargo/registry,sharing=locked \
    --mount=type=cache,target=/opt/rust/cargo/git,sharing=locked \
    set -e; \
    . /tmp/rust-versions.env; \
    curl --proto '=https' --tlsv1.2 -sSf "https://sh.rustup.rs" | sh -s -- -y --default-toolchain "${RUST_STABLE_VERSION}" --profile "minimal" --no-modify-path && \
    echo 'export PATH="${CARGO_HOME}/bin:${PATH}"' > "/etc/profile.d/cargo.sh" && \
    . "/etc/profile.d/cargo.sh" && \
    rustup toolchain install "${RUST_NIGHTLY_VERSION}" && \
    rustup toolchain link nightly "${RUSTUP_HOME}/toolchains/${RUST_NIGHTLY_VERSION}-$(rustc -vV | grep host | cut -d' ' -f2)" && \
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
      mkdir -p "/usr/share/keyrings" && \
      curl -sSL "https://apt.llvm.org/llvm-snapshot.gpg.key" | gpg --dearmor > "/usr/share/keyrings/llvm-archive-keyring.gpg" && \
      echo "deb [signed-by=/usr/share/keyrings/llvm-archive-keyring.gpg] http://apt.llvm.org/trixie/ llvm-toolchain-trixie main" > "/etc/apt/sources.list.d/llvm.list" && \
      echo "deb [signed-by=/usr/share/keyrings/llvm-archive-keyring.gpg] http://apt.llvm.org/trixie/ llvm-toolchain-trixie-21 main" >> "/etc/apt/sources.list.d/llvm.list" && \
      apt-get update && \
      apt-get install --no-install-recommends -y \
        "clang-21" "clang-tools-21" "clang-format-21" "clang-tidy-21" \
        "libclang-rt-21-dev" "lld-21" "lldb-21" \
        "libc++-21-dev" "libc++abi-21-dev" \
        "llvm-21" "llvm-21-dev" "llvm-21-runtime" \
        "clang-format-21" && \
      update-alternatives --install "/usr/bin/clang" clang "/usr/bin/clang-21" 100 && \
      update-alternatives --install "/usr/bin/clang++" clang++ "/usr/bin/clang++-21" 100 && \
      update-alternatives --install "/usr/bin/clang-format" clang-format "/usr/bin/clang-format-21" 100 && \
      update-alternatives --install "/usr/bin/clang-tidy" clang-tidy "/usr/bin/clang-tidy-21" 100 && \
      update-alternatives --install "/usr/bin/lldb" lldb "/usr/bin/lldb-21" 100 && \
      update-alternatives --install "/usr/bin/ld.lld" ld.lld "/usr/bin/ld.lld-21" 100; \
      update-alternatives --install "/usr/bin/llvm-symbolizer" llvm-symbolizer "/usr/bin/llvm-symbolizer-21" 100 && \
      update-alternatives --install "/usr/bin/llvm-config" llvm-config "/usr/bin/llvm-config-21" 100 && \
      update-alternatives --install "/usr/bin/clang-format" clang-format "/usr/bin/clang-format-21" 100; \
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
      "imagemagick" "python3" "python3-pip" "python3-venv" \
      "vim" "bsdmainutils" "strace" "pngtools" "ripgrep" "exiftool" && \
    echo 'export PATH="${HOME}/.local/share/pnpm:${PATH}"' >> "/etc/bash.bashrc" && \
    pip3 install --resume-retries=5 --no-cache-dir "numpy" "opencv-python" "Pillow" "scikit-image" "flake8" "black" && \
    update-alternatives --install "/usr/bin/python" python "$(command -v "python3")" 100 && \
    . "/etc/profile.d/cargo.sh" && \
    rm -f /tmp/rust-versions.env

ENV EDITOR="vim"
ENV VISUAL="vim"

COPY .devcontainer/resources/.bashrc /root/.bashrc
COPY .devcontainer/resources/bin/* /usr/local/bin/
