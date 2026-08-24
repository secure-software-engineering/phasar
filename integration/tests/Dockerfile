# syntax=docker/dockerfile:1
#
# Test environment for the PhasarToolchain.cmake integration tests.
#
# The toolchain is the runtime function here: the tests check whether CMake,
# clang, wllvm and the LLVM binutils together produce a whole-program module.
# Hence a single stage that deliberately ships compilers in the runtime image.
#
#   docker compose up --attach-dependencies tests
#
# The build context is integration/, so that both the toolchain under test and
# tests/ end up in the image.
ARG debian_digest=sha256:abd67ffcfa541b485a3dff59865ab629aa048a6c613e639d36e7456b0b229241
FROM debian:12-slim@${debian_digest}

ARG llvm_version=22
ARG wllvm_version=1.3.1
# Debian 12 ships CMake 3.25; C++20 modules need 3.28 or newer, and the projects
# under test include one that uses them.
ARG cmake_version=3.28.4
ARG cmake_sha256_wheel=bc06b483cb2bcaf78184e66ac7fd0f3bbe4ed9a690369b1e4897b19568d566dd
ARG cmake_sha256_sdist=b9dd1010ebe951e1acce054c295d5f891a8ae12b295d158b66020c955ae861b4

# Without this the Debian image configuration discards downloaded packages and
# renders the APT cache mount useless.
RUN set -eux; \
    rm -f /etc/apt/apt.conf.d/docker-clean; \
    echo 'Binary::apt::APT::Keep-Downloaded-Packages "true";' \
        > /etc/apt/apt.conf.d/keep-cache;

# llvm-<N> provides llvm-link/llvm-ar/llvm-dis for extract-bc — clang-<N> alone
# does not. clang-tools-<N> provides clang-scan-deps for C++20 modules; for LLVM
# 22 it happens to sit in clang-22, for LLVM 20 it does not. wllvm needs `file` to
# stamp the .llvm_bc section.
RUN --mount=type=bind,from=phasar_utils,source=InstallAptDependencies.sh,target=/usr/local/bin/install_apt_dependencies.sh \
    --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    set -eux; \
    install_apt_dependencies.sh --noninteractive --llvm-version "${llvm_version}" \
        "llvm-${llvm_version}" "clang-tools-${llvm_version}" file python3-venv;

# tests/tooling/ rather than tests/build/: the repository's root .gitignore
# excludes build*/, which would silently drop this script from the checkout.
RUN --mount=type=bind,source=tests/tooling/install_wllvm.sh,target=/usr/local/bin/install_wllvm.sh \
    set -eux; \
    install_wllvm.sh "${wllvm_version}" \
        eaf9e319a8d6b56c0f43dbe118823c9e1901e5a733f4e34d44ffc26852f62091 \
        3e057a575f05c9ecc8669a8c4046f2bfdf0c69533b87b4fbfcabe0df230cc331;

RUN --mount=type=bind,source=tests/tooling/install_cmake.sh,target=/usr/local/bin/install_cmake.sh \
    set -eux; \
    install_cmake.sh "${cmake_version}" \
        "${cmake_sha256_wheel}" "${cmake_sha256_sdist}";

ENV PATH=/opt/cmake/bin:/opt/wllvm/bin:$PATH

# The toolchain prefers CMAKE_C_COMPILER and $CC over its own search. Both stay
# unset so the tests actually exercise the selection logic.
#
# No USER here on purpose: the entrypoint discovers the owner of the mounted
# workspace and drops to it, which needs root for exactly that one step. It runs
# nothing as root itself.
COPY --chown=1000:1000 . /work

WORKDIR /work
ENTRYPOINT ["tests/tooling/entrypoint.sh"]
