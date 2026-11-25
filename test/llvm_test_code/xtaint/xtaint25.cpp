[[clang::annotate("psr.source")]] int source() { return 42; }
void sink([[clang::annotate("psr.sink")]] int) {}
class Sanitizer {
public:
  virtual void sanitize([[clang::annotate("psr.sanitizer")]] int) noexcept {}
};

Sanitizer *makeSanitizer() { return new Sanitizer; }
extern void disposeSanitizer(Sanitizer *sanitizer);

int main() {
  auto srcOne = source();
  auto srcTwo = source();

  auto *SanitizerObj = makeSanitizer();
  SanitizerObj->sanitize(srcOne);

  sink(srcOne);
  sink(srcTwo);

  disposeSanitizer(SanitizerObj);
}

// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %llvm_test_code/xtaint/xtaint25_cpp_dbg.ll | FileCheck %s -check-prefix=ide-xtaint
// ide-xtaint: /xtaint/xtaint25.cpp:19:3:

// RUN: %phasar-cli --data-flow-analysis=ifds-taint --module %llvm_test_code/xtaint/xtaint25_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: /xtaint/xtaint25.cpp:18:3:
// ifds-taint: /xtaint/xtaint25.cpp:19:3:

// RUN: %phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %llvm_test_code/xtaint/xtaint25_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: /xtaint/xtaint25.cpp:19:3:

// RUN: %phasar-cli --data-flow-analysis=monoifds-taint --module %llvm_test_code/xtaint/xtaint25_cpp_dbg.ll | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: /xtaint/xtaint25.cpp:19:3:

// RUN: %phasar-cli --data-flow-analysis=sparse-ifds-taint --module %llvm_test_code/xtaint/xtaint25_cpp_dbg.ll | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: /xtaint/xtaint25.cpp:18:3:
// sparse-ifds-taint: /xtaint/xtaint25.cpp:19:3:
