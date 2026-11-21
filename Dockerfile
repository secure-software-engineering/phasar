ARG baseimage="ubuntu:26.04"
FROM "$baseimage" AS build

RUN --mount=type=bind,source=./utils/InstallAptDependencies.sh,target=/InstallAptDependencies.sh \
  set -eux; \
  ./InstallAptDependencies.sh --noninteractive tzdata clang-21 libclang-rt-21-dev clang-tools-21 llvm-21-tools

ENV CC=/usr/bin/clang-21 \
    CXX=/usr/bin/clang++-21

FROM build

ARG RUN_TESTS=OFF
RUN --mount=type=bind,source=.,target=/usr/src/phasar,rw \
  set -eux; \
  cd /usr/src/phasar; \
  if find external -mindepth 1 -maxdepth 1 -type d -empty | grep -q .; then \
    echo "external/* submodules are not checked out; run 'git submodule update --init --recursive' on the host before building"; \
    exit 1; \
  fi; \
  cmake -S . -B cmake-build/Release \
    -DCMAKE_BUILD_TYPE=Release \
    -DPHASAR_TARGET_ARCH="" \
    -DPHASAR_ENABLE_SANITIZERS=ON \
    -DPHASAR_USE_Z3=ON \
    -DPHASAR_BUILD_UNITTESTS=$RUN_TESTS \
    -DPHASAR_BUILD_IR=$RUN_TESTS \
    -DPHASAR_ENABLE_INTEGRATIONTESTS=$RUN_TESTS \
    -DPHASAR_BUILD_OPENSSL_TS_UNITTESTS=OFF \
    -G Ninja; \
  ninja -C cmake-build/Release install; \
  [ "${RUN_TESTS}" = "OFF" ] || cmake --build ./cmake-build/Release --target check-phasar-unittests check-phasar-cli ; \
  phasar-cli --version

ENTRYPOINT [ "phasar-cli" ]
