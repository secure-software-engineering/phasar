NUM_THREADS=$(nproc)
LLVM_INSTALL_DIR="/usr/local/llvm-15"
LLVM_RELEASE=llvmorg-15.0.7

# installing LLVM
tmp_dir=$(mktemp -d "llvm-build.XXXXXXXX" --tmpdir)
./utils/install-llvm.sh "${NUM_THREADS}" "${tmp_dir}" ${LLVM_INSTALL_DIR} ${LLVM_RELEASE}
rm -rf "${tmp_dir}"
