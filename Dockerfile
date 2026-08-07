ARG baseimage="ubuntu:24.04"
# LLVM/clang major version used throughout the image. This is the version PhASAR
# links against, so it is also the LLVM IR version phasar-cli can parse and the
# version WLLVM must emit. Bump both ARGs together to change it.
ARG llvm_version=22
# PhASAR's cmake version string. LLVM 22 releases as 22.1.x (new scheme), whereas
# LLVM <=20 use NN.0.x — so this is "22.1" for llvm 22, but e.g. "16" for llvm 16.
ARG phasar_llvm_version=22.1

FROM "$baseimage" AS build

ARG llvm_version

# Install the LLVM/clang toolchain plus the pieces the whole-program wrapper needs:
# `llvm-<v>` provides llvm-link/llvm-dis used by WLLVM, `python3-pip` installs wllvm.
# `lld-<v>` is LLVM's linker: PhASAR's Release build uses ThinLTO, whose bitcode
# objects the default GNU ld (gold plugin) fails to link on LLVM 22 — lld links
# them natively, so we keep LTO enabled and link with it (see -fuse-ld=lld below).
RUN --mount=type=bind,source=./utils/InstallAptDependencies.sh,target=/InstallAptDependencies.sh \
  set -eux; \
  ./InstallAptDependencies.sh --noninteractive --llvm-version "${llvm_version}" \
    tzdata "clang-tools-${llvm_version}" "llvm-${llvm_version}" "lld-${llvm_version}" python3-pip file; \
  pip3 install --no-cache-dir --break-system-packages wllvm

ENV CC=/usr/bin/clang-${llvm_version} \
    CXX=/usr/bin/clang++-${llvm_version}

FROM build

ARG llvm_version
ARG phasar_llvm_version

ARG RUN_TESTS=OFF
RUN --mount=type=bind,source=.,target=/usr/src/phasar,rw \
  set -eux; \
  cd /usr/src/phasar; \
  git submodule update --init; \
  cmake -S . -B cmake-build/Release \
    -DCMAKE_BUILD_TYPE=Release \
    -DPHASAR_LLVM_VERSION="${phasar_llvm_version}" \
    -DPHASAR_TARGET_ARCH="" \
    -DPHASAR_ENABLE_SANITIZERS=ON \
    -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
    -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld" \
    -DCMAKE_MODULE_LINKER_FLAGS="-fuse-ld=lld" \
    -DPHASAR_USE_Z3=ON \
    -DPHASAR_BUILD_UNITTESTS=$RUN_TESTS \
    -DPHASAR_BUILD_IR=$RUN_TESTS \
    -DPHASAR_BUILD_OPENSSL_TS_UNITTESTS=OFF \
    -G Ninja; \
  ninja -C cmake-build/Release install; \
  [ "${RUN_TESTS}" = "ON" ] && ctest --test-dir cmake-build/Release --output-on-failure || true; \
  phasar-cli --version

# Install the end-to-end wrapper: build a C/C++ project to whole-program IR via
# WLLVM and run a PhASAR analysis in one command. `phasar-analyze cli ...` still
# exposes the raw phasar-cli.
COPY utils/phasar-analyze.sh /usr/local/bin/phasar-analyze
RUN chmod +x /usr/local/bin/phasar-analyze

# LLVM version WLLVM must emit for phasar-cli to accept the bitcode. Kept in sync
# with the toolchain above via the llvm_version ARG. Override at runtime with
# `-e PHASAR_IR_LLVM_VERSION=NN`.
ENV PHASAR_IR_LLVM_VERSION=${llvm_version}

ENTRYPOINT [ "phasar-analyze" ]
