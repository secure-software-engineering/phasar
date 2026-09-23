
void print([[clang::annotate("psr.sink")]] int) {}

int main([[clang::annotate("psr.source")]] int argc, char *argv[]) {

  int array[2];
  array[1] = argc;

  print(array[0]);
  print(array[1]);
}

// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %llvm_test_code/xtaint/xtaint03_cpp_dbg.ll | FileCheck %s -check-prefix=ide-xtaint
// ide-xtaint: /xtaint/xtaint03.cpp:10:3:

// RUN: %phasar-cli --data-flow-analysis=ifds-taint --module %llvm_test_code/xtaint/xtaint03_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: /xtaint/xtaint03.cpp:9:3:
// ifds-taint: /xtaint/xtaint03.cpp:10:3:

// RUN: %phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %llvm_test_code/xtaint/xtaint03_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: /xtaint/xtaint03.cpp:10:3:

// RUN: %phasar-cli --data-flow-analysis=monoifds-taint --module %llvm_test_code/xtaint/xtaint03_cpp_dbg.ll | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: /xtaint/xtaint03.cpp:9:3:
// monoifds-taint: /xtaint/xtaint03.cpp:10:3:

// RUN: %phasar-cli --data-flow-analysis=sparse-ifds-taint --module %llvm_test_code/xtaint/xtaint03_cpp_dbg.ll | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: /xtaint/xtaint03.cpp:10:3:
