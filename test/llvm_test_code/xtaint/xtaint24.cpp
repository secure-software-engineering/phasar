[[clang::annotate("psr.source")]] int source() { return 42; }
void sanitize([[clang::annotate("psr.sanitizer")]] int) noexcept {}
class Sink {
public:
  virtual void sink([[clang::annotate("psr.sink")]] int) {}
};

Sink *makeSink() { return new Sink; }
extern void disposeSink(Sink *snk);

int main() {
  auto srcOne = source();
  auto srcTwo = source();

  sanitize(srcOne);

  auto *SinkObj = makeSink();
  SinkObj->sink(srcOne);
  SinkObj->sink(srcTwo);

  disposeSink(SinkObj);
  disposeSink(SinkObj);
}

// RUN: %phasar-cli --data-flow-analysis=ide-xtaint --module %llvm_test_code/xtaint/xtaint24_cpp_dbg.ll | FileCheck %s -check-prefix=ide-xtaint
// ide-xtaint: /xtaint/xtaint24.cpp:19:12:

// RUN: %phasar-cli --data-flow-analysis=ifds-taint --module %llvm_test_code/xtaint/xtaint24_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-taint
// ifds-taint: /xtaint/xtaint24.cpp:18:12:
// ifds-taint: /xtaint/xtaint24.cpp:19:12:

// RUN: %phasar-cli --data-flow-analysis=ifds-fieldsens-taint --module %llvm_test_code/xtaint/xtaint24_cpp_dbg.ll | FileCheck %s -check-prefix=ifds-fieldsens-taint
// ifds-fieldsens-taint: /xtaint/xtaint24.cpp:19:12:

// RUN: %phasar-cli --data-flow-analysis=monoifds-taint --module %llvm_test_code/xtaint/xtaint24_cpp_dbg.ll | FileCheck %s -check-prefix=monoifds-taint
// monoifds-taint: /xtaint/xtaint24.cpp:19:12:

// RUN: %phasar-cli --data-flow-analysis=sparse-ifds-taint --module %llvm_test_code/xtaint/xtaint24_cpp_dbg.ll | FileCheck %s -check-prefix=sparse-ifds-taint
// sparse-ifds-taint: /xtaint/xtaint24.cpp:18:12:
// sparse-ifds-taint: /xtaint/xtaint24.cpp:19:12:
