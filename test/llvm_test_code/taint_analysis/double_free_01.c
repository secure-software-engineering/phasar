#include <stdlib.h>

int main() {
  void *X = malloc(32);
  free(X);
  free(X);
}

// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %llvm_test_code/taint_analysis/double_free_01_c_dbg.ll --analysis-config %S/../../../config/double-free-config.json | FileCheck %s -check-prefix=xtaint
// xtaint: /taint_analysis/double_free_01.c:6:3:

// RUN: %phasar-cli --data-flow-analysis=ifds-taint --module %llvm_test_code/taint_analysis/double_free_01_c_dbg.ll --analysis-config %S/../../../config/double-free-config.json | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: /taint_analysis/double_free_01.c:6:3:

// RUN: %phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %llvm_test_code/taint_analysis/double_free_01_c_dbg.ll --analysis-config %S/../../../config/double-free-config.json | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: /taint_analysis/double_free_01.c:6:3:

// RUN: %phasar-cli --data-flow-analysis=monoifds-taint --module %llvm_test_code/taint_analysis/double_free_01_c_dbg.ll --analysis-config %S/../../../config/double-free-config.json | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: A LLVM-based static analysis framework

// RUN: %phasar-cli --data-flow-analysis=sparse-ifds-taint --module %llvm_test_code/taint_analysis/double_free_01_c_dbg.ll --analysis-config %S/../../../config/double-free-config.json | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: A LLVM-based static analysis framework
