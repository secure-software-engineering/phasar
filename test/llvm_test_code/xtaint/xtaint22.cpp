
void print([[clang::annotate("psr.sink")]] int) {}

int main([[clang::annotate("psr.source")]] int argc, char *argv[]) {
  int arr[10]{};
  arr[4] = argc;

  for (int *it = arr, *end = arr + 10; it != end; ++it) {
    print(*it);
  }
}

// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %llvm_test_code/xtaint/xtaint22_cpp_dbg.ll | FileCheck %s -check-prefix=ide-xtaint
// ide-xtaint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=ifds-taint --module %llvm_test_code/xtaint/xtaint22_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: /xtaint/xtaint22.cpp:9:5:

// RUN: %phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %llvm_test_code/xtaint/xtaint22_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: /xtaint/xtaint22.cpp:9:5:

// RUN: %phasar-cli --data-flow-analysis=monoifds-taint --module %llvm_test_code/xtaint/xtaint22_cpp_dbg.ll | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=sparse-ifds-taint --module %llvm_test_code/xtaint/xtaint22_cpp_dbg.ll | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: /xtaint/xtaint22.cpp:9:5:
