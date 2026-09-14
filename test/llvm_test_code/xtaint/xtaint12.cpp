[[clang::annotate("psr.source")]] extern int source() { return 0; }
void sink([[clang::annotate("psr.sink")]] int) {}
void sanitize([[clang::annotate("psr.sanitizer")]] int &) noexcept {}

struct IntPair {
  int x;
  int y;
};

int *getPtr(int **pptaint) { return *pptaint; }

int main() {
  int taint = source();
  int *ptaint = &taint;

  sanitize(*getPtr(&ptaint));

  sink(*getPtr(&ptaint));
}

// RUN: %S/../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ide-xtaint --module %S/../../../build/test/llvm_test_code/xtaint/xtaint12_cpp_dbg.ll | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=ide-xtaint
// ide-xtaint: /xtaint/xtaint12.cpp:18:3:

// RUN: %S/../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ifds-taint --module %S/../../../build/test/llvm_test_code/xtaint/xtaint12_cpp_dbg.ll | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=ifds-taint
// ifds-taint: /xtaint/xtaint12.cpp:18:3:

// RUN: %S/../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %S/../../../build/test/llvm_test_code/xtaint/xtaint12_cpp_dbg.ll | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: /xtaint/xtaint12.cpp:18:3:

// RUN: %S/../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=monoifds-taint --module %S/../../../build/test/llvm_test_code/xtaint/xtaint12_cpp_dbg.ll | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: /xtaint/xtaint12.cpp:18:3:

// RUN: %S/../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=sparse-ifds-taint --module %S/../../../build/test/llvm_test_code/xtaint/xtaint12_cpp_dbg.ll | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: /xtaint/xtaint12.cpp:18:3:
