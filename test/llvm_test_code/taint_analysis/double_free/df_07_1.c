#include <stdlib.h>

typedef struct _S {
  int *XY[2];
} S;

void v(int **foo) { //
  free(foo[-1]);
}
void f(int **foo) { v(foo); }
int main() {
  S foo = {};
  foo.XY[0] = (int *)malloc(32);
  foo.XY[1] = (int *)malloc(32);

  free(foo.XY[0]);
  f(&foo.XY[1]);
  return 0;
}

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ide-xtaint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_07_1_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=xtaint
// xtaint: No leaks found!

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_07_1_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: /taint_analysis/double_free/df_07_1.c:8:3:

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_07_1_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: /taint_analysis/double_free/df_07_1.c:8:3:

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=monoifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_07_1_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: /taint_analysis/double_free/df_07_1.c:8:3:

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=sparse-ifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_07_1_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: /taint_analysis/double_free/df_07_1.c:8:3:
