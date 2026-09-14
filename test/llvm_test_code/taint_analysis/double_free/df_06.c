
/// TODO: Fields

#include <stdlib.h>

typedef struct _S {
  int *X;
  int *Y;
} S;

void v(S *foo) {
  free(foo->Y); // foo->X is free'd, but foo->Y is not, so this is fine
}
void f(S *foo) { v(foo); }
int main() {
  S foo = {};
  foo.X = (int *)malloc(32);
  foo.Y = (int *)malloc(32);

  free(foo.X);
  f(&foo);
  return 0;
}

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ide-xtaint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_06_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=xtaint
// xtaint: No leaks found!

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_06_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=ifds-taint
// ifds-taint: /taint_analysis/double_free/df_06.c:12:3:

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_06_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: /taint_analysis/double_free/df_06.c:12:3:

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=monoifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_06_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: /taint_analysis/double_free/df_06.c:12:3:

// RUN: %S/../../../../build/tools/phasar-cli/phasar-cli --data-flow-analysis=sparse-ifds-taint --module %S/../../../../build/test/llvm_test_code/taint_analysis/double_free/df_06_c_dbg.ll --analysis-config %S/../../../../config/double-free-config.json | /usr/local/llvm-16/bin/FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: /taint_analysis/double_free/df_06.c:12:3:
