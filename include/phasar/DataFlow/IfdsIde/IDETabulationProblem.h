/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_DATAFLOW_IFDSIDE_IDETABULATIONPROBLEM_H_
#define PHASAR_DATAFLOW_IFDSIDE_IDETABULATIONPROBLEM_H_

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctions.h"
#include "phasar/DataFlow/IfdsIde/EntryPointUtils.h"
#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/IFDSIDESolverConfig.h"
#include "phasar/DataFlow/IfdsIde/IfdsIdeDomain.h"
#include "phasar/DataFlow/IfdsIde/InitialSeeds.h"
#include "phasar/DataFlow/IfdsIde/Solver/GenericSolverResults.h"
#include "phasar/Utils/DefaultAnalysisPrinterSelector.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/NullAnalysisPrinter.h"
#include "phasar/Utils/SemiRing.h"
#include "phasar/Utils/Soundness.h"

#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

namespace psr {

struct HasNoConfigurationType;

template <IdeAnalysisDomain AnalysisDomainTy> class AllTopFnProvider {
public:
  virtual ~AllTopFnProvider() = default;
  /// Returns an edge function that represents the top element of the analysis.
  virtual EdgeFunction<typename AnalysisDomainTy::l_t> allTopFunction() = 0;
};

template <IdeAnalysisDomain AnalysisDomainTy>
  requires HasJoinLatticeTraits<typename AnalysisDomainTy::l_t>
class AllTopFnProvider<AnalysisDomainTy> {
public:
  virtual ~AllTopFnProvider() = default;
  /// Returns an edge function that represents the top element of the analysis.
  virtual EdgeFunction<typename AnalysisDomainTy::l_t> allTopFunction() {
    return AllTop<typename AnalysisDomainTy::l_t>{};
  }
};

/// \brief The analysis problem interface for IDE problems (solvable by the
/// IDESolver). Create a subclass from this and override all pure-virtual
/// functions to create your own IDE analysis.
///
/// For more information on how to write an IDE analysis, see [Writing an IDE
/// Analysis](https://github.com/secure-software-engineering/phasar/wiki/Writing-an-IDE-analysis)
template <IdeAnalysisDomain AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IDETabulationProblem : public FlowFunctions<AnalysisDomainTy, Container>,
                             public EdgeFunctions<AnalysisDomainTy>,
                             public JoinLattice<AnalysisDomainTy>,
                             public SemiRing<AnalysisDomainTy>,
                             public AllTopFnProvider<AnalysisDomainTy> {
public:
  using ProblemAnalysisDomain = AnalysisDomainTy;
  using d_t = typename AnalysisDomainTy::d_t;
  using n_t = typename AnalysisDomainTy::n_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;
  using l_t = typename AnalysisDomainTy::l_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using db_t = typename AnalysisDomainTy::db_t;

  using ConfigurationTy = HasNoConfigurationType;

  /// Takes an IR database (IRDB) and collects information from it to create a
  /// tabulation problem.
  /// @param[in] IRDB The project IR database, that holds the code under
  /// analysis
  /// @param[in] EntryPoints The (mangled) names of all entry functions of the
  /// target being analyzed, given as a vector of strings. An example would
  /// simply be `{"main"}`. To set every function as entry point, pass
  /// `"__ALL__"`
  /// @param[in] ZeroValue Provides the special tautological zero value (aka.
  /// Λ). If not provided here, you must set it via \link initializeZeroValue()
  /// \endlink.
  explicit IDETabulationProblem(
      const db_t *IRDB, std::vector<std::string> EntryPoints,
      std::optional<d_t>
          ZeroValue) noexcept(std::is_nothrow_move_constructible_v<d_t>)
      : IRDB(IRDB), EntryPoints(std::move(EntryPoints)),
        ZeroValue(std::move(ZeroValue)),
        Printer(std::make_unique<typename DefaultAnalysisPrinterSelector<
                    AnalysisDomainTy>::type>()) {
    assert(IRDB != nullptr);
  }

  IDETabulationProblem(IDETabulationProblem &&) noexcept = default;
  IDETabulationProblem &operator=(IDETabulationProblem &&) noexcept = default;

  IDETabulationProblem(const IDETabulationProblem &) = delete;
  IDETabulationProblem &operator=(const IDETabulationProblem &) = delete;

  ~IDETabulationProblem() override = default;

  void
  setAnalysisPrinter(MaybeUniquePtr<AnalysisPrinterBase<AnalysisDomainTy>> P) {
    if (P) {
      Printer = std::move(P);
    } else {
      Printer = NullAnalysisPrinter<AnalysisDomainTy>::getInstance();
    }
  }

  [[nodiscard]] constexpr AnalysisPrinterBase<AnalysisDomainTy> &
  printer() noexcept {
    assert(Printer != nullptr);
    return *Printer;
  }

  [[nodiscard]] constexpr MaybeUniquePtr<AnalysisPrinterBase<AnalysisDomainTy>>
  consumePrinter() noexcept {
    assert(Printer != nullptr);
    return std::exchange(Printer,
                         NullAnalysisPrinter<AnalysisDomainTy>::getInstance());
  }

  /// Checks if the given data-flow fact is the special tautological lambda (or
  /// zero) fact.
  [[nodiscard]] virtual bool isZeroValue(d_t FlowFact) const noexcept {
    assert(ZeroValue.has_value());
    return FlowFact == *ZeroValue;
  }

  /// Returns initial seeds to be used for the analysis. This is a mapping of
  /// statements to initial analysis facts.
  [[nodiscard]] virtual InitialSeeds<n_t, d_t, l_t> initialSeeds() = 0;

  /// Returns the special tautological lambda (or zero) fact.
  [[nodiscard]] ByConstRef<d_t> getZeroValue() const {
    assert(ZeroValue.has_value());
    return *ZeroValue;
  }

  void initializeZeroValue(d_t Zero) noexcept(
      std::is_nothrow_assignable_v<std::optional<d_t> &, d_t &&>) {
    assert(!ZeroValue.has_value());
    ZeroValue = std::move(Zero);
  }

  /// Sets the configuration to be used by the IFDS/IDE solver.
  void setIFDSIDESolverConfig(IFDSIDESolverConfig Config) noexcept {
    SolverConfig = Config;
  }

  /// Returns the configuration of the IFDS/IDE solver.
  [[nodiscard]] IFDSIDESolverConfig &getIFDSIDESolverConfig() noexcept {
    return SolverConfig;
  }

  /// Generates a text report of the results that is written to the specified
  /// output stream.
  virtual void
  emitTextReport([[maybe_unused]] GenericSolverResults<n_t, d_t, l_t> Results,
                 llvm::raw_ostream &OS = llvm::outs()) {
    Printer->onFinalize(OS);
  }

  /// Generates a graphical report, e.g. in html or other markup languages, of
  /// the results that is written to the specified output stream.
  virtual void emitGraphicalReport(
      [[maybe_unused]] GenericSolverResults<n_t, d_t, l_t> Results,
      llvm::raw_ostream &OS = llvm::outs()) {
    OS << "No graphical report available!\n";
  }

  /// Sets the level of soundness to be used by the analysis. Returns false if
  /// the level of soundness is ignored. Otherwise, true.
  virtual bool setSoundness(Soundness /*S*/) { return false; }

  [[nodiscard]] const db_t *getProjectIRDB() const noexcept { return IRDB; }

  [[nodiscard]] llvm::ArrayRef<std::string> getEntryPoints() const noexcept {
    return EntryPoints;
  }

protected:
  typename FlowFunctions<AnalysisDomainTy, Container>::FlowFunctionPtrType
  generateFromZero(d_t FactToGenerate) {
    return FlowFunctions<AnalysisDomainTy, Container>::generateFlow(
        std::move(FactToGenerate), getZeroValue());
  }

  template <typename D = d_t, typename L = l_t>
  void onResult(n_t Instr, D &&DfFact, L &&LatticeElement,
                DataFlowAnalysisType AnalysisType) {
    Printer->onResult(Instr, PSR_FWD(DfFact), PSR_FWD(LatticeElement),
                      AnalysisType);
  }

  template <typename D = d_t>
    requires std::is_same_v<l_t, BinaryDomain>
  void onResult(n_t Instr, D &&DfFact, DataFlowAnalysisType AnalysisType) {
    Printer->onResult(Instr, PSR_FWD(DfFact), AnalysisType);
  }

  /// Seeds that just start with ZeroValue and bottomElement() at the starting
  /// points of each EntryPoint function.
  /// Takes the __ALL__ EntryPoint into account.
  [[nodiscard]] InitialSeeds<n_t, d_t, l_t> createDefaultSeeds()
    requires std::is_nothrow_default_constructible_v<
        typename AnalysisDomainTy::c_t>
  {
    InitialSeeds<n_t, d_t, l_t> Seeds;
    typename AnalysisDomainTy::c_t C{};

    addSeedsForStartingPoints(EntryPoints, IRDB, C, Seeds, getZeroValue(),
                              this->bottomElement());

    return Seeds;
  }

  const db_t *IRDB{};
  std::vector<std::string> EntryPoints;
  std::optional<d_t> ZeroValue;

  IFDSIDESolverConfig SolverConfig{};

  [[maybe_unused]] Soundness SF = Soundness::Soundy;

  MaybeUniquePtr<AnalysisPrinterBase<AnalysisDomainTy>> Printer;
};

} // namespace psr

#endif
