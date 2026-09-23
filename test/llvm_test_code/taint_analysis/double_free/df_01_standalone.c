#include <stdlib.h>
int main() {
  int *foo = (int *)malloc(32);
  free(foo);
  free(foo); // vulnerability
  return 0;
}


// This file is not integrated in the CMakeLists.txt; below run-commands compile it on-the-fly

// RUN: %clang %s -S -emit-llvm -g -w -o %t
// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %t --analysis-config %config/double-free-config.json | FileCheck %s

// CHECK: taint_analysis/double_free/df_01_standalone.c:5:3:
