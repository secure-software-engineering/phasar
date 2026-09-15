#include <stdlib.h>

int *rec(int *p, int n) { return n > 0 ? rec(p, n - 1) : p; }

int main() {
  int *foo = (int *)malloc(32);
  free(foo);

  int *x = rec(foo, 3);
  free(x);

  return 0;
}

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ide-xtaint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_ctx_01_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=xtaint
// xtaint: /taint_analysis/double_free/df_ctx_01.c:10:3:

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_ctx_01_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: No leaks found!

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_ctx_01_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: No leaks found!

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=monoifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_ctx_01_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: /taint_analysis/double_free/df_ctx_01.c:10:3:

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=sparse-ifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_ctx_01_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: No leaks found!
