extern void srcsink(
    [[clang::annotate("psr.sink")]] [[clang::annotate("psr.source")]] int &) {}
void sink([[clang::annotate("psr.sink")]] int) {}

int main(int argc, char *argv[]) {
  int x = 42;
  int y = 24;

  srcsink(x);
  srcsink(y);

  srcsink(x); // leak
  sink(y);    // leak
}

// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %llvm_test_code/xtaint/xtaint20_cpp_dbg.ll | FileCheck %s -check-prefix=ide-xtaint
// ide-xtaint: /xtaint/xtaint20.cpp:12:3:
// ide-xtaint: /xtaint/xtaint20.cpp:13:3:

// RUN: %phasar-cli --data-flow-analysis=ifds-taint --module %llvm_test_code/xtaint/xtaint20_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: /xtaint/xtaint20.cpp:10:3:
// ifds-taint: /xtaint/xtaint20.cpp:12:3:
// ifds-taint: /xtaint/xtaint20.cpp:13:3:

// RUN: %phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %llvm_test_code/xtaint/xtaint20_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: /xtaint/xtaint20.cpp:10:3:
// ifds-fieldsens-taint: /xtaint/xtaint20.cpp:12:3:
// ifds-fieldsens-taint: /xtaint/xtaint20.cpp:13:3:

// RUN: %phasar-cli --data-flow-analysis=monoifds-taint --module %llvm_test_code/xtaint/xtaint20_cpp_dbg.ll | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: /xtaint/xtaint20.cpp:12:3:
// monoifds-taint: /xtaint/xtaint20.cpp:13:3:

// RUN: %phasar-cli --data-flow-analysis=sparse-ifds-taint --module %llvm_test_code/xtaint/xtaint20_cpp_dbg.ll | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: /xtaint/xtaint20.cpp:10:3:
// sparse-ifds-taint: /xtaint/xtaint20.cpp:12:3:
// sparse-ifds-taint: /xtaint/xtaint20.cpp:13:3:
