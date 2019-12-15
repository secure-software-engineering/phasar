#include <gtest/gtest.h>
#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedVariationalICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDEVariabilityTabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h>
#include <phasar/PhasarLLVM/Passes/ValueAnnotationPass.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>

#include <tuple>

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IDEVariabilityTabulationProblemTest : public ::testing::Test {
protected:
  const std::string pathToLLFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/linear_constant/";
  const std::set<std::string> EntryPoints = {"main"};

  // Function - Line Nr - Variable - Value
  using LCACompactResult_t =
      std::tuple<std::string, std::size_t, std::string, int64_t>;
  ProjectIRDB *IRDB = nullptr;

  void SetUp() override { boost::log::core::get()->set_logging_enabled(false); }

  IDELinearConstantAnalysis::lca_restults_t
  doAnalysis(const std::string &llvmFilePath, bool printDump = false) {
    IRDB = new ProjectIRDB({pathToLLFiles + llvmFilePath}, IRDBOptions::WPA);
    ValueAnnotationPass::resetValueID();
    LLVMTypeHierarchy TH(*IRDB);
    LLVMPointsToInfo PT(*IRDB);
    LLVMBasedICFG ICFG(TH, *IRDB, CallGraphAnalysisType::OTF, EntryPoints);
    LLVMBasedVariationalICFG VICFG(TH, *IRDB, CallGraphAnalysisType::OTF,
                                   EntryPoints);
    IDELinearConstantAnalysis LCAProblem(IRDB, &TH, &ICFG, &PT, EntryPoints);
    IDEVariabilityTabulationProblem<
        IDELinearConstantAnalysis::n_t, IDELinearConstantAnalysis::d_t,
        IDELinearConstantAnalysis::m_t, IDELinearConstantAnalysis::t_t,
        IDELinearConstantAnalysis::v_t, IDELinearConstantAnalysis::l_t,
        IDELinearConstantAnalysis::i_t>
        vara(LCAProblem, VICFG);
    IDESolver<IDELinearConstantAnalysis::n_t, IDELinearConstantAnalysis::d_t,
              IDELinearConstantAnalysis::m_t, IDELinearConstantAnalysis::t_t,
              IDELinearConstantAnalysis::v_t, IDELinearConstantAnalysis::l_t,
              VariationalICFG<IDELinearConstantAnalysis::n_t,
                              IDELinearConstantAnalysis::m_t, z3::expr>>
        LCASolver(vara);
    LCASolver.solve();
    if (printDump) {
      LCASolver.dumpResults();
    }
    return LCAProblem.getLCAResults(LCASolver.getSolverResults());
  }

  void TearDown() override { delete IRDB; }

  /**
   * We map instruction id to value for the ground truth. ID has to be
   * a string since Argument ID's are not integer type (e.g. main.0 for argc).
   * @param groundTruth results to compare against
   * @param solver provides the results
   */
  void compareResults(IDELinearConstantAnalysis::lca_restults_t &Results,
                      std::set<LCACompactResult_t> &GroundTruth) {
    std::set<LCACompactResult_t> RelevantResults;
    for (auto G : GroundTruth) {
      std::string fName = std::get<0>(G);
      unsigned line = std::get<1>(G);
      if (Results.find(fName) != Results.end()) {
        if (auto it = Results[fName].find(line); it != Results[fName].end()) {
          for (auto varToVal : it->second.variableToValue) {
            RelevantResults.emplace(fName, line, varToVal.first,
                                    varToVal.second);
          }
        }
      }
    }
    EXPECT_EQ(RelevantResults, GroundTruth);
  }
}; // Test Fixture

// main function for the test case/*  */
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}