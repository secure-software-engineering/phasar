#include "phasar/Utils/ErrorFwd.h"

#include "llvm/ADT/Twine.h"
#include "llvm/Support/ErrorHandling.h"

void psr::composeEFPureVirtualError(llvm::StringRef ConcreteEF,
                                    llvm::StringRef L) {
  llvm::report_fatal_error(
      "EdgeFunction composition is not implemented for " + ConcreteEF +
      "; Either implement static EdgeFunction<" + L +
      "> compose(const EdgeFunctionRef<" + ConcreteEF +
      ">, const EdgeFunction<" + L + ">&) in " + ConcreteEF +
      ", or override EdgeFunction<" + L + "> extend(const EdgeFunction<" + L +
      ">&, const EdgeFunction<" + L + ">&) in your IDETabulationProblem");
}

void psr::joinEFPureVirtualError(llvm::StringRef ConcreteEF,
                                 llvm::StringRef L) {
  llvm::report_fatal_error(
      "EdgeFunction join is not implemented for " + ConcreteEF +
      "; Either implement static EdgeFunction<" + L +
      "> join(const EdgeFunctionRef<" + ConcreteEF + ">, const EdgeFunction<" +
      L + ">&) in " + ConcreteEF + ", or override EdgeFunction<" + L +
      "> combine(const EdgeFunction<" + L + ">&, const EdgeFunction<" + L +
      ">&) in your IDETabulationProblem");
}
