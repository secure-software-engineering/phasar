void sink([[clang::annotate("psr.sink")]] int) {}
void sanitize([[clang::annotate("psr.sanitizer")]] int *) noexcept {}
class Source {
public:
  [[clang::annotate("psr.source")]] virtual int get() { return 42; }
};

Source *makeSource() { return new Source; }
extern void disposeSource(Source *src);

struct DoubleIntPair {
  double d;
  int i;
};

int main() {
  auto src = makeSource();
  DoubleIntPair dip = {3.1415926, src->get()};

  auto x = dip.i;
  sanitize(&dip.i);

  sink(dip.i);
  sink(x);

  disposeSource(src);
}

// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %llvm_test_code/xtaint/xtaint15_cpp_dbg.ll | FileCheck %s -check-prefix=ide-xtaint
// ide-xtaint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=ifds-taint --module %llvm_test_code/xtaint/xtaint15_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %llvm_test_code/xtaint/xtaint15_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=monoifds-taint --module %llvm_test_code/xtaint/xtaint15_cpp_dbg.ll | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: No leaks found!

// RUN: %phasar-cli --data-flow-analysis=sparse-ifds-taint --module %llvm_test_code/xtaint/xtaint15_cpp_dbg.ll | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: No leaks found!
