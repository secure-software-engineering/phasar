#include <stdlib.h>

int main() {
  int *foo = (int *)malloc(32);
  free(foo);
  if (rand())
    free(foo); // vulnerability
  return 0;
}

// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %llvm_test_code/taint_analysis/double_free/df_08_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=xtaint
// xtaint: /taint_analysis/double_free/df_08.c:7:5:

// RUN: %phasar-cli --data-flow-analysis=ifds-taint --module %llvm_test_code/taint_analysis/double_free/df_08_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: /taint_analysis/double_free/df_08.c:7:5:

// RUN: %phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %llvm_test_code/taint_analysis/double_free/df_08_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: /taint_analysis/double_free/df_08.c:7:5:

// RUN: %phasar-cli --data-flow-analysis=monoifds-taint --module %llvm_test_code/taint_analysis/double_free/df_08_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: /taint_analysis/double_free/df_08.c:7:5:

// RUN: %phasar-cli --data-flow-analysis=sparse-ifds-taint --module %llvm_test_code/taint_analysis/double_free/df_08_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: /taint_analysis/double_free/df_08.c:7:5:
