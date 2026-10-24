#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/Solver/GenericSolverResults.h"
#include "phasar/Domain/AnalysisDomain.h"
#include "phasar/Utils/AnalysisPrinterBase.h"
#include "phasar/Utils/DefaultAnalysisPrinterSelector.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/NullAnalysisPrinter.h"

namespace psr {
template <typename AnalysisDomainTy> class WithAnalysisPrinterMixin {
  // NOTE: Don't add type-aliases for n_t,d_t, ... here; this will break the
  // compilation with gcc; gcc apparently ignores private visibility when
  // checking for ambiguous declarations. clang instead behaves as expected.

public:
  WithAnalysisPrinterMixin()
      : Printer(std::make_unique<typename DefaultAnalysisPrinterSelector<
                    AnalysisDomainTy>::type>()) {}

  WithAnalysisPrinterMixin(
      MaybeUniquePtr<AnalysisPrinterBase<AnalysisDomainTy>> Printer) noexcept
      : Printer(std::move(Printer)) {}

  constexpr auto setAnalysisPrinter(
      MaybeUniquePtr<AnalysisPrinterBase<AnalysisDomainTy>> P) noexcept {
    if (P) {
      return std::exchange(Printer, std::move(P));
    }

    return std::exchange(Printer,
                         NullAnalysisPrinter<AnalysisDomainTy>::getInstance());
  }

  [[nodiscard]] constexpr auto consumePrinter() noexcept {
    return setAnalysisPrinter(nullptr);
  }

  [[nodiscard]] constexpr AnalysisPrinterBase<AnalysisDomainTy> &
  printer() noexcept {
    assert(Printer != nullptr);
    return *Printer;
  }

  void emitTextReport(
      [[maybe_unused]] GenericSolverResultsFor<AnalysisDomainTy> Results,
      llvm::raw_ostream &OS = llvm::outs()) {
    Printer->onFinalize(OS);
  }

protected:
  template <typename D = AnalysisDomainTy::d_t,
            typename L = detail::ValueDomainAdder<AnalysisDomainTy>::l_t>
  void onResult(AnalysisDomainTy::n_t Instr, D &&DfFact, L &&LatticeElement,
                DataFlowAnalysisType AnalysisType) {
    Printer->onResult(Instr, PSR_FWD(DfFact), PSR_FWD(LatticeElement),
                      AnalysisType);
  }

  template <typename D = AnalysisDomainTy::d_t>
    requires std::is_same_v<
        typename detail::ValueDomainAdder<AnalysisDomainTy>::l_t, BinaryDomain>
  void onResult(AnalysisDomainTy::n_t Instr, D &&DfFact,
                DataFlowAnalysisType AnalysisType) {
    Printer->onResult(Instr, PSR_FWD(DfFact), AnalysisType);
  }

private:
  MaybeUniquePtr<AnalysisPrinterBase<AnalysisDomainTy>> Printer;
};
} // namespace psr
