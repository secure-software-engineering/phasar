extern void srcsink(int &);
extern void sink(int);

int main(int argc, char *argv[]) {
  // The configuration is provided as callback
  //
  // PHASAR_DECLARE_FUN_AS_SINK(srcsink, 0);
  // PHASAR_DECLARE_FUN_AS_SINK(sink, 0);
  // PHASAR_DECLARE_FUN_AS_SOURCE(srcsink, false, 0);

  int x = 42;
  int y = 24;

  srcsink(x);
  srcsink(y);

  srcsink(x); // leak
  sink(y);    // leak
}

// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %llvm_test_code/xtaint/xtaint21_cpp_dbg.ll | FileCheck %s -check-prefix=ide-xtaint
// ide-xtaint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=ifds-taint --module %llvm_test_code/xtaint/xtaint21_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %llvm_test_code/xtaint/xtaint21_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=monoifds-taint --module %llvm_test_code/xtaint/xtaint21_cpp_dbg.ll | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=sparse-ifds-taint --module %llvm_test_code/xtaint/xtaint21_cpp_dbg.ll | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: No leaks found!
