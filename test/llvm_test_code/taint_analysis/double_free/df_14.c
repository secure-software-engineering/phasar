#include <stdlib.h>

int main() {
  int *Arr[] = {
      (int *)malloc(4),
      (int *)malloc(4),
  };

  for (int **it = Arr, **end = it + 2; it != end; ++it) {
    free(*it); // no vulnerability -- fp
  }

  return 0;
}

// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %llvm_test_code/taint_analysis/double_free/df_14_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=xtaint
// xtaint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=ifds-taint --module %llvm_test_code/taint_analysis/double_free/df_14_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %llvm_test_code/taint_analysis/double_free/df_14_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=monoifds-taint --module %llvm_test_code/taint_analysis/double_free/df_14_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: /taint_analysis/double_free/df_14.c:10:5:

// RUN: %phasar-cli --data-flow-analysis=sparse-ifds-taint --module %llvm_test_code/taint_analysis/double_free/df_14_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: No leaks found!
