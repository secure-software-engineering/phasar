#include <stdlib.h>

int *g;

void v(void) { free(g); }

int main() {
  g = (int *)malloc(32);

  free(g);
  v();
  return 0;
}

// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %llvm_test_code/taint_analysis/double_free/df_11_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=xtaint
// xtaint: /taint_analysis/double_free/df_11.c:5:16:

// RUN: %phasar-cli --data-flow-analysis=ifds-taint --module %llvm_test_code/taint_analysis/double_free/df_11_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: /taint_analysis/double_free/df_11.c:5:16:

// RUN: %phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %llvm_test_code/taint_analysis/double_free/df_11_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: /taint_analysis/double_free/df_11.c:5:16:

// RUN: %phasar-cli --data-flow-analysis=monoifds-taint --module %llvm_test_code/taint_analysis/double_free/df_11_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: /taint_analysis/double_free/df_11.c:5:16:

// RUN: %phasar-cli --data-flow-analysis=sparse-ifds-taint --module %llvm_test_code/taint_analysis/double_free/df_11_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: /taint_analysis/double_free/df_11.c:5:16:
