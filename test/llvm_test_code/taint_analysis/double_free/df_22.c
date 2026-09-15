#include <stdlib.h>

int *v(int *p) { return p; }

int main() {
  int *foo = (int *)malloc(32);
  free(foo);
  int *x = v(foo);

  int a = 42;
  int *y = v(&a);

  free(x);
  free(y);

  return 0;
}

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ide-xtaint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_22_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=xtaint
// xtaint: /taint_analysis/double_free/df_22.c:13:3:

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_22_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: /taint_analysis/double_free/df_22.c:13:3:

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_22_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: /taint_analysis/double_free/df_22.c:13:3:

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=monoifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_22_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: /taint_analysis/double_free/df_22.c:13:3:

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=sparse-ifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_22_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: /taint_analysis/double_free/df_22.c:13:3:
