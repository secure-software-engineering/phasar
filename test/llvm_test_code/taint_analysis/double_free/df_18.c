#include <stdlib.h>

void *alloc() { //
  return malloc(4);
}

int main() {
  void *p = alloc();
  free(p);
  p = alloc();
  free(p);

  return 0;
}

// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %llvm_test_code/taint_analysis/double_free/df_18_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=xtaint
// xtaint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=ifds-taint --module %llvm_test_code/taint_analysis/double_free/df_18_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %llvm_test_code/taint_analysis/double_free/df_18_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=monoifds-taint --module %llvm_test_code/taint_analysis/double_free/df_18_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: /taint_analysis/double_free/df_18.c:11:3:

// RUN: %phasar-cli --data-flow-analysis=sparse-ifds-taint --module %llvm_test_code/taint_analysis/double_free/df_18_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: No leaks found!
