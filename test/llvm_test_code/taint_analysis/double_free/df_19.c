#include <stdlib.h>

void **alloc() { //
  return (void **)malloc(sizeof(void *));
}

int main() {
  void **p = alloc();
  *p = malloc(4);
  free(*p);
  free(*p);

  return 0;
}

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ide-xtaint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_19_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=xtaint
// xtaint: /taint_analysis/double_free/df_19.c:11:3:

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_19_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=ifds-taint
// ifds-taint: No leaks found!

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_19_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: No leaks found!

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=monoifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_19_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: /taint_analysis/double_free/df_19.c:11:3:

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=sparse-ifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_19_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: No leaks found!
