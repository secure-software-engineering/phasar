#include "phasar/Utils/InitPhasar.h"

#include "phasar/Utils/Logger.h"

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"

using namespace psr;

#define PSR_ISSUE_TRACKER_URL                                                  \
  "https://github.com/secure-software-engineering/phasar/issues"

InitPhasar::InitPhasar(int &Argc, const char **&Argv) noexcept
    : llvm::InitLLVM(Argc, Argv) {
  static std::nullptr_t InitGlobals = [] {
    // Replace LLVM's bug report URL with ours
    llvm::setBugReportMsg("PLEASE create a bug report at " PSR_ISSUE_TRACKER_URL
                          " and include the crash backtrace.\n ");

    // Install custom error handlers, such that fatal errors do not start with
    // "LLVM ERROR"
    auto FatalErrorHandler = [](void * /*HandlerData*/, const char *Reason,
                                bool /*GenCrashDiag*/) {
      // Prevent recursion in case our error handler itself fails
      llvm::remove_fatal_error_handler();
      llvm::remove_bad_alloc_error_handler();

      // Write the actual error message
      Logger::addLinePrefix(llvm::errs(), SeverityLevel::CRITICAL,
                            std::nullopt);
      llvm::errs() << Reason << '\n';
      llvm::errs().flush();
    };

    // NOTE: Install the bad_alloc handler before the fatal_error handler due to
    // a bug in LLVM
    // https://github.com/llvm/llvm-project/issues/83040
    llvm::install_bad_alloc_error_handler(FatalErrorHandler, nullptr);
    llvm::install_fatal_error_handler(FatalErrorHandler, nullptr);
    // llvm::install_out_of_memory_new_handler() is already done by InitLLVM
    return nullptr;
  }();
  (void)InitGlobals;
}
