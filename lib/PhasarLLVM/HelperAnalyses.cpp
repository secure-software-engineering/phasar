#include "phasar/PhasarLLVM/HelperAnalyses.h"

#include "phasar/ControlFlow/CGSCCs.h"
#include "phasar/DataFlow/HelperAnalyses.h"
#include "phasar/PhasarLLVM/ControlFlow/EntryFunctionUtils.h"
#include "phasar/PhasarLLVM/ControlFlow/FunctionCompressor.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCallGraphBuilder.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/RTAResolver.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSetData.h"
#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/UsedGlobals.h"
#include "phasar/Pointer/AliasAnalysisType.h"

#include <memory>
#include <string>

using namespace psr;

static_assert(CanGetICFG<HelperAnalyses>);
static_assert(CanGetICFGOf<HelperAnalyses, const llvm::Instruction *,
                           const llvm::Function *>);
static_assert(
    CanGetCompressedFunctionsOf<HelperAnalyses, const llvm::Function *>);
static_assert(CanGetCGSCCs<HelperAnalyses>);

HelperAnalyses::HelperAnalyses(std::string IRFile,
                               std::optional<LLVMAliasSetData> PrecomputedPTS,
                               AliasAnalysisType PTATy, bool AllowLazyPTS,
                               std::vector<std::string> EntryPoints,
                               std::optional<CallGraphData> PrecomputedCG,
                               CallGraphAnalysisType CGTy,
                               Soundness SoundnessLevel,
                               bool AutoGlobalSupport) noexcept
    : IRFile(std::move(IRFile)), PrecomputedPTS(std::move(PrecomputedPTS)),
      PTATy(PTATy), AllowLazyPTS(AllowLazyPTS),
      PrecomputedCG(std::move(PrecomputedCG)),
      EntryPoints(std::move(EntryPoints)), CGTy(CGTy),
      SoundnessLevel(SoundnessLevel), AutoGlobalSupport(AutoGlobalSupport) {}

HelperAnalyses::HelperAnalyses(std::string IRFile,
                               std::vector<std::string> EntryPoints,
                               HelperAnalysisConfig Config) noexcept
    : IRFile(std::move(IRFile)),
      PrecomputedPTS(std::move(Config.PrecomputedPTS)), PTATy(Config.PTATy),
      UFAATy(Config.UFAATy), AllowLazyPTS(Config.AllowLazyPTS),
      PrecomputedCG(std::move(Config.PrecomputedCG)),
      EntryPoints(std::move(EntryPoints)), CGTy(Config.CGTy),
      SoundnessLevel(Config.SoundnessLevel),
      AutoGlobalSupport(Config.AutoGlobalSupport) {}

HelperAnalyses::HelperAnalyses(const llvm::Twine &IRFile,
                               std::vector<std::string> EntryPoints,
                               HelperAnalysisConfig Config)
    : HelperAnalyses(IRFile.str(), std::move(EntryPoints), std::move(Config)) {}
HelperAnalyses::HelperAnalyses(const char *IRFile,
                               std::vector<std::string> EntryPoints,
                               HelperAnalysisConfig Config)
    : HelperAnalyses(std::string(IRFile), std::move(EntryPoints),
                     std::move(Config)) {}
HelperAnalyses::HelperAnalyses(llvm::Module *IRModule,
                               std::vector<std::string> EntryPoints,
                               HelperAnalysisConfig Config)
    : HelperAnalyses(std::string(), std::move(EntryPoints), std::move(Config)) {
  this->IRDB = std::make_unique<LLVMProjectIRDB>(
      IRModule, Config.PreprocessExistingModule);
}
HelperAnalyses::HelperAnalyses(std::unique_ptr<llvm::Module> IRModule,
                               std::vector<std::string> EntryPoints,
                               HelperAnalysisConfig Config)
    : HelperAnalyses(std::string(), std::move(EntryPoints), std::move(Config)) {
  this->IRDB = std::make_unique<LLVMProjectIRDB>(
      std::move(IRModule), Config.PreprocessExistingModule);
}

HelperAnalyses::~HelperAnalyses() noexcept = default;

LLVMProjectIRDB &HelperAnalyses::getProjectIRDB() {
  if (!IRDB) {
    IRDB = std::make_unique<LLVMProjectIRDB>(IRFile);
  }
  return *IRDB;
}

LLVMAliasInfoRef HelperAnalyses::getAliasInfo() {
  if (!PT) {
    if (PrecomputedPTS.has_value()) {
      PT = std::make_unique<LLVMAliasSet>(&getProjectIRDB(), *PrecomputedPTS);
    } else if (PTATy == AliasAnalysisType::UnionFind) {
      auto &IRDB = getProjectIRDB();
      auto VTP = LLVMVFTableProvider(IRDB);
      auto Res = RTAResolver(&IRDB, &VTP, &getTypeHierarchy());
      const auto BaseCG = buildLLVMBasedCallGraph(
          IRDB, Res, getEntryFunctions(IRDB, EntryPoints));
      PT = std::make_unique<LLVMUnionFindAliasSet>(
          &getProjectIRDB(), BaseCG,
          LLVMUnionFindAliasSet::Config{
              .AType = UFAATy,
              .ALocality = LLVMUnionFindAliasSet::AnalysisLocality::Global,
          });
    } else {
      PT = std::make_unique<LLVMAliasSet>(&getProjectIRDB(), AllowLazyPTS,
                                          PTATy);
    }
  }
  return PT.get();
}

DIBasedTypeHierarchy &HelperAnalyses::getTypeHierarchy() {
  if (!TH) {
    TH = std::make_unique<DIBasedTypeHierarchy>(getProjectIRDB());
  }
  return *TH;
}

LLVMBasedICFG &HelperAnalyses::getICFG() {
  if (!ICF) {
    if (PrecomputedCG.has_value()) {
      ICF = std::make_unique<LLVMBasedICFG>(&getProjectIRDB(), *PrecomputedCG);
    } else {
      ICF = std::make_unique<LLVMBasedICFG>(
          &getProjectIRDB(), CGTy, std::move(EntryPoints), &getTypeHierarchy(),
          needsAliasInfo(CGTy) ? getAliasInfo() : nullptr, SoundnessLevel,
          AutoGlobalSupport);
    }
  }

  return *ICF;
}

LLVMBasedCFG &HelperAnalyses::getCFG() {
  if (!CFG) {
    if (ICF) {
      return *ICF;
    }
    CFG = std::make_unique<LLVMBasedCFG>();
  }
  return *CFG;
}

FunctionCompressor<const llvm::Function *> &
HelperAnalyses::getCompressedFunctions() {
  if (!FC) {
    auto Funs = compressFunctions(
        getICFG().getCallGraph(),
        psr::getEntryFunctions(getProjectIRDB(), EntryPoints));
    FC = std::make_unique<FunctionCompressor<const llvm::Function *>>(
        std::move(Funs));
  }

  return *FC;
}

const SCCHolder<FunctionId> &HelperAnalyses::getCGSCCs() {
  if (!SCCs) {
    auto &ICF = getICFG();
    auto CGSCCs = computeCGSCCs(ICF, getCompressedFunctions());
    SCCs = std::make_unique<SCCHolder<FunctionId>>(std::move(CGSCCs));
  }
  return *SCCs;
}

const SCCDependencyGraph<FunctionId> &HelperAnalyses::getCGSCCCallers() {
  if (!SCCCallers) {
    auto SCCC =
        computeCGSCCCallers(getICFG(), getCompressedFunctions(), getCGSCCs());
    SCCCallers =
        std::make_unique<SCCDependencyGraph<FunctionId>>(std::move(SCCC));
  }
  return *SCCCallers;
}

const UsedGlobalsHolder<const llvm::GlobalVariable *> &
HelperAnalyses::getUsedGlobals() {
  if (!UsedGlobals) {
    auto UG = computeUsedGlobals(getProjectIRDB(), getCompressedFunctions(),
                                 getCGSCCs(), getCGSCCCallers());
    UsedGlobals =
        std::make_unique<UsedGlobalsHolder<const llvm::GlobalVariable *>>(
            std::move(UG));
  }
  return *UsedGlobals;
}
