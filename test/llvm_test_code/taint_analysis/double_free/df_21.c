#include <stdlib.h>
// void v(int **foo) {
//   free(*foo); // vulnerability
// }

// void v(void *dataVoidPtr) {
//   /* cast void pointer to a pointer of the appropriate type */
//   char **dataPtr = (char **)dataVoidPtr;
//   /* dereference dataPtr into data */
//   char *data = (*dataPtr);
//   /* POTENTIAL FLAW: Possibly freeing memory twice */
//   free(data);
// }

void v(void **dataVoidPtr) {
  /* cast void pointer to a pointer of the appropriate type */
  // char **dataPtr = (char **)dataVoidPtr;
  /* dereference dataPtr into data */
  // void *data = (*dataVoidPtr);
  /* POTENTIAL FLAW: Possibly freeing memory twice */
  free(*dataVoidPtr);
}

int main() {
  int *foo = (int *)malloc(32);
  free(foo);
  v(&foo);
  return 0;
}

// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %llvm_test_code/taint_analysis/double_free/df_21_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=xtaint
// xtaint: /taint_analysis/double_free/df_21.c:21:3:

// RUN: %phasar-cli --data-flow-analysis=ifds-taint --module %llvm_test_code/taint_analysis/double_free/df_21_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: /taint_analysis/double_free/df_21.c:21:3:

// RUN: %phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %llvm_test_code/taint_analysis/double_free/df_21_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: /taint_analysis/double_free/df_21.c:21:3:

// RUN: %phasar-cli --data-flow-analysis=monoifds-taint --module %llvm_test_code/taint_analysis/double_free/df_21_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=sparse-ifds-taint --module %llvm_test_code/taint_analysis/double_free/df_21_c_dbg.ll --analysis-config %config/double-free-config.json | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: /taint_analysis/double_free/df_21.c:21:3:
