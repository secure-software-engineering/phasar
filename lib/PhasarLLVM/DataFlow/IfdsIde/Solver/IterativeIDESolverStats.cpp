#include "phasar/DataFlow/IfdsIde/Solver/IterativeIDESolverStats.h"

#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   const IterativeIDESolverStats &S) {
  OS << S.FEStats;

  OS << "General Solver Stats:\n";
  OS << "> AllInterPropagations:\t\t" << S.NumAllInterPropagations << '\n';
  OS << "> AllInterPropagations(Bytes):\t~" << S.AllInterPropagationsBytes
     << '\n';
  OS << "> SourceFactAndCSToInterJob:\t" << S.SourceFactAndCSToInterJobSize
     << '\n';
  OS << "> SourceFactAndCSToInterJob(Bytes):\t~"
     << S.SourceFactAndCSToInterJobBytes << '\n';
  OS << "> SourceFactAndFuncToInterJob:\t" << S.SourceFactAndFuncToInterJobSize
     << '\n';
  OS << "> SourceFactAndFuncToInterJob(Bytes):\t~"
     << S.SourceFactAndFuncToInterJobBytes << '\n';
  OS << "> MaxInnerMapSize:\t" << S.MaxESGEdgesPerInst << '\n';
  OS << "> AvgInnerMapSize:\t" << llvm::format("%g", S.AvgESGEdgesPerInst)
     << '\n';
  OS << "> JumpFunctions Map Size(Bytes):\t~" << S.JumpFunctionsMapBytes
     << '\n';
  OS << "> ValTab Size(Bytes):\t\t~" << S.ValTabBytes << '\n';
  OS << "> NumFlowFacts:\t\t" << S.NumFlowFacts << '\n';

  OS << "Compressor Capacities:\n";
  OS << "> NodeCompressor:\t" << S.InstCompressorCapacity << '\n';
  OS << "> FactCompressor:\t" << S.FactCompressorCapacity << '\n';
  OS << "> FunCompressor:\t" << S.FunCompressorCapacity << '\n';

  OS << "High Watermarks:\n";
  OS << "> JumpFunctions:\t" << S.NumPathEdgesHighWatermark << '\n';
  OS << "> WorkList:\t" << S.WorkListHighWatermark << '\n';
  OS << "> CallWL:\t" << S.CallWLHighWatermark << '\n';
  OS << "> WLProp:\t" << S.WLPropHighWatermark << '\n';
  OS << "> WLComp:\t" << S.WLCompHighWatermark << '\n';

  OS << "InterPropagationJobs:\n";
  OS << "> Total calls to propagateProcedureSummaries: "
     << S.TotalNumRelevantCalls << '\n';
  OS << "> Max InterJobs per relevant call: "
     << S.MaxNumInterJobsPerRelevantCall << '\n';
  OS << "> Avg InterJobs per relevant call: "
     << llvm::format("%g", double(S.CumulNumInterJobsPerRelevantCall) /
                               double(S.TotalNumRelevantCalls))
     << '\n';
  OS << "> Total num of linear searches for summaries: "
     << S.TotalNumLinearSearchForSummary << '\n';
  OS << "> Max Length of linear search for summaries: "
     << S.MaxLenLinearSearchForSummary << '\n';
  OS << "> Avg Length of linear search for summaries: "
     << llvm::format("%g", double(S.CumulLinearSearchLenForSummary) /
                               double(S.TotalNumLinearSearchForSummary))
     << '\n';
  OS << "> Max Diff of summaries found vs search length: "
     << S.MaxDiffNumSummariesFound << '\n';
  OS << "> Avg Diff of summaries found vs search length: "
     << llvm::format("%g", double(S.CumulDiffNumSummariesFound) /
                               double(S.TotalNumLinearSearchForSummary))
     << '\n';
  OS << "> Rel Diff of summaries found vs search length: "
     << llvm::format("%g", double(S.CumulRelDiffNumSummariesFound) /
                               double(S.TotalNumLinearSearchForSummary))
     << '\n';
  OS << "> Num Cached EndSummaries: " << S.NumEndSummaries << '\n';
  OS << "> EndSummaryTab(Bytes): " << S.EndSummaryTabSize << '\n';

  return OS;
}
