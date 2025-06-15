#include "phasar/DataFlow.h"               // For solveIFDSProblem()
#include "phasar/PhasarLLVM/ControlFlow.h" // For the LLVMBasedICFG
#include "phasar/PhasarLLVM/DB.h"          // For the LLVMProjectIRDB
#include "phasar/PhasarLLVM/DataFlow.h" // For DefaultAliasAwareIFDSProblem, etc.
#include "phasar/PhasarLLVM/Pointer.h"  // For the LLVMAliasSet
#include "phasar/PhasarLLVM/TaintConfig.h" // For the LLVMTaintConfig

namespace {

void populateWithMayAliases(psr::LLVMAliasInfoRef AS,
                            std::set<const llvm::Value *> &Facts);

/// To create a custom IFDS analysis, we must create a subclass of the
/// IFDSTabulationProblem.
/// The utility class DefaultAliasAwareIFDSProblem implements
/// IFDSTabulationProblem and already provides some default flow-functions and
/// handles aliasing, so that we can focus on the specifica of our analysis.
class ExampleTaintAnalysis : public psr::DefaultAliasAwareIFDSProblem {
public:
  /// Constructor of the taint-analysis problem. Just forward all parameters to
  /// the base-class and initialize the taint-config.
  ///
  /// The last parameter of the base-ctor denotes the special zero-value of the
  /// IFDS problem. We use LLVMZeroValue for this.
  explicit ExampleTaintAnalysis(const psr::LLVMProjectIRDB *IRDB,
                                psr::LLVMAliasInfoRef AS,
                                const psr::LLVMTaintConfig *Config,
                                std::vector<std::string> EntryPoints)
      : psr::DefaultAliasAwareIFDSProblem(IRDB, AS, std::move(EntryPoints),
                                          psr::LLVMZeroValue::getInstance()),
        Config(&psr::assertNotNull(Config)) {}

  /// Provides the initial seeds, i.e., the <stmt, fact> pairs that are assumed
  /// to hold un-conditionally at the beginning of the analysis.
  /// This is the start state that the IFDS solver will use to start with.
  [[nodiscard]] psr::InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    psr::InitialSeeds<n_t, d_t, l_t> Seeds;

    psr::LLVMBasedCFG CFG;
    // Here, we just say that for all entry-functions in the EntryPoints, the
    // zero-value should hold at the very first statement.
    addSeedsForStartingPoints(EntryPoints, IRDB, CFG, Seeds, getZeroValue());

    return Seeds;
  };

  /// Here, we define special semantics of function-calls that are specified
  /// outside of the target program. In the case of taint analysis, we need to
  /// handle sources, sinks and sanitizers here:
  [[nodiscard]] FlowFunctionPtrType
  getSummaryFlowFunction(n_t CallSite, f_t DestFun) override {
    const auto *CS = llvm::cast<llvm::CallBase>(CallSite);

    // Process the effects of source or sink functions that are called
    auto Gen = psr::getGeneratedFacts<container_type>(*Config, CS, DestFun);
    auto Leak = psr::getLeakedFacts<container_type>(*Config, CS, DestFun);
    auto Kill = psr::getSanitizedFacts<container_type>(*Config, CS, DestFun);

    if (Gen.empty() && Leak.empty() && Kill.empty()) {
      // This CallSite apparently is not calling a special source/sink/sanitizer
      // function. Fallback to the default-behavior.
      return DefaultAliasAwareIFDSProblem::getSummaryFlowFunction(CS, DestFun);
    }

    // Since our analysis is alias-aware, we must handle aliasing here:
    populateWithMayAliases(getAliasInfo(), Gen);
    populateWithMayAliases(getAliasInfo(), Leak);

    // We have special behavior to communicate to the analysis solver, so create
    // a flow-function that captures this behavior:
    return lambdaFlow([this, CS, Gen{std::move(Gen)}, Leak{std::move(Leak)},
                       Kill{std::move(Kill)}](d_t Source) -> container_type {
      if (isZeroValue(Source)) {
        // In case of a source, we generate the new taints from zero (Source).
        return Gen;
      }

      if (Leak.count(Source)) {
        // In case of a sink, we create a leak if one of the leaking parameters
        // (Leak) is tainted (Source).
        Leaks.insert(CS);
      }

      if (Kill.count(Source)) {
        // In case of a sanitizer, we kill tainted values (Source) that flow
        // into the sanitizied parameters (Kill).
        return {};
      }

      // Otherwise, the taint is unaffected from the source/sink/sanitizer, so
      // propagate it as identity
      return {Source};
    });
  }

  // We collect the leaking sink-calls here
  llvm::DenseSet<n_t> Leaks{};

private:
  const psr::LLVMTaintConfig *Config{};
};

// For all given facts, we add their aliases:
void populateWithMayAliases(psr::LLVMAliasInfoRef AS,
                            std::set<const llvm::Value *> &Facts) {
  auto Tmp = Facts;
  for (const auto *Fact : Facts) {
    auto Aliases = AS.getAliasSet(Fact);
    Tmp.insert(Aliases->begin(), Aliases->end());
  }

  Facts = std::move(Tmp);
}

} // namespace

// Invoke the analysis the same way as explained in 04-run-ifds-analysis:
int main(int Argc, char *Argv[]) {
  if (Argc < 2) {
    llvm::errs() << "USAGE: write-ifds-analysis-simple <LLVM-IR file>\n";
    return 1;
  }

  // Load the IR
  psr::LLVMProjectIRDB IRDB(Argv[1]);
  if (!IRDB) {
    return 1;
  }

  psr::LLVMAliasSet AS(&IRDB);
  psr::LLVMTaintConfig TC(IRDB);
  TC.print();
  llvm::outs() << "------------------------\n";

  ExampleTaintAnalysis TaintProblem(&IRDB, &AS, &TC, {"main"});

  psr::LLVMBasedICFG ICFG(&IRDB, psr::CallGraphAnalysisType::OTF, {"main"},
                          nullptr, &AS);

  psr::solveIFDSProblem(TaintProblem, ICFG);

  for (const auto *LeakInst : TaintProblem.Leaks) {
    llvm::outs() << "Detected taint leak at " << psr::llvmIRToString(LeakInst)
                 << '\n';
  }
}
