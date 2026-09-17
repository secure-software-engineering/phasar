
void print([[clang::annotate("psr.sink")]] int) {
  /// TODO: clang::annotate does not work with function declarations (e.g.
  /// extern functions or forward references)
}

int main([[clang::annotate("psr.source")]] int argc, char *argv[]) {
  print(argc);
}

// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %llvm_test_code/xtaint/xtaint01_cpp_dbg.ll | FileCheck %s -check-prefix=ide-xtaint
// ide-xtaint: /xtaint/xtaint01.cpp:8:3:

// RUN: %phasar-cli --data-flow-analysis=ifds-taint --module %llvm_test_code/xtaint/xtaint01_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: /xtaint/xtaint01.cpp:8:3:

// RUN: %phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %llvm_test_code/xtaint/xtaint01_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: /xtaint/xtaint01.cpp:8:3:

// RUN: %phasar-cli --data-flow-analysis=monoifds-taint --module %llvm_test_code/xtaint/xtaint01_cpp_dbg.ll | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=sparse-ifds-taint --module %llvm_test_code/xtaint/xtaint01_cpp_dbg.ll | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: /xtaint/xtaint01.cpp:8:3:
