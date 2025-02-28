ARG baseimage="ubuntu:24.04"
FROM "$baseimage" as build

RUN --mount=type=bind,source=./utils/InstallAptDependencies.sh,target=/InstallAptDependencies.sh \
  set -eux; \
  ./InstallAptDependencies.sh --noninteractive tzdata clang-19 libclang-rt-19-dev

ENV CC=/usr/bin/clang-19 \
    CXX=/usr/bin/clang++-19

FROM build

ARG RUN_TESTS=OFF
RUN --mount=type=bind,source=.,target=/usr/src/phasar,rw \
  set -eux; \
  cd /usr/src/phasar; \
  git submodule update --init; \
  cmake -S . -B cmake-build/Release \
    -DCMAKE_BUILD_TYPE=Release \
    -DPHASAR_TARGET_ARCH="" \
    -DPHASAR_ENABLE_SANITIZERS=ON \
    -DBUILD_PHASAR_CLANG=ON \
    -DPHASAR_USE_Z3=ON \
    -DPHASAR_BUILD_UNITTESTS=$RUN_TESTS \
    -DPHASAR_BUILD_IR=$RUN_TESTS \
    -DPHASAR_BUILD_OPENSSL_TS_UNITTESTS=OFF \
    -G Ninja; \
  ninja -C cmake-build/Release install; \
  [ "${RUN_TESTS}" = "ON" ] && ctest --test-dir cmake-build/Release --output-on-failure || true; \
  phasar-cli --version

ENTRYPOINT [ "phasar-cli" ]
