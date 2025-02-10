#include "SVFBasedAliasAnalysis.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/AliasAnalysisView.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/AliasResult.h"

#include "DDA/ContextDDA.h"
#include "DDA/DDAClient.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "SVFIR/SVFIR.h"
#include "SVFIR/SVFModule.h"
#include "SVFIR/SVFType.h"
#include "WPA/Andersen.h"
#include "WPA/VersionedFlowSensitive.h"

#include <memory>
#include <optional>
#include <stdexcept>

namespace psr {
static constexpr psr::AliasResult
translateSVFAliasResult(SVF::AliasResult AR) noexcept {
  switch (AR) {
  case SVF::NoAlias:
    return AliasResult::NoAlias;
  case SVF::MayAlias:
    return AliasResult::MayAlias;
  case SVF::MustAlias:
    return AliasResult::MustAlias;
  case SVF::PartialAlias:
    return AliasResult::PartialAlias;
  }
}
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class SVFAliasAnalysisBase : public AliasAnalysisView {
public:
  SVFAliasAnalysisBase(SVF::SVFModule *Mod, AliasAnalysisType PATy)
      : AliasAnalysisView(PATy), IRBuilder(Mod), PAG(IRBuilder.build()) {}

  ~SVFAliasAnalysisBase() override {
    SVF::SVFIR::releaseSVFIR();
    SVF::AndersenWaveDiff::releaseAndersenWaveDiff();
    SVF::SymbolTableInfo::releaseSymbolInfo();
    SVF::LLVMModuleSet::releaseLLVMModuleSet();
  }

private:
  void doErase(llvm::Function *F) noexcept override {}
  void doClear() noexcept override {}

protected:
  SVF::SVFIRBuilder IRBuilder;
  SVF::SVFIR *PAG;
};

class SVFVFSAnalysis : public SVFAliasAnalysisBase {
public:
  SVFVFSAnalysis(SVF::SVFModule *Mod)
      : SVFAliasAnalysisBase(Mod, AliasAnalysisType::SVFVFS), VFS(PAG) {}

private:
  static psr::AliasResult aliasImpl(void *AACtx, const llvm::Value *V,
                                    const llvm::Value *Rep,
                                    const llvm::DataLayout & /*DL*/) {
    auto *ModSet = SVF::LLVMModuleSet::getLLVMModuleSet();
    auto *Nod1 = ModSet->getSVFValue(V);
    auto *Nod2 = ModSet->getSVFValue(Rep);

    if (!Nod1 || !Nod2) {
      return AliasResult::MayAlias;
    }

    auto *AA = static_cast<SVFVFSAnalysis *>(AACtx);
    return translateSVFAliasResult(AA->VFS.alias(Nod1, Nod2));
  }

  FunctionAliasView doGetAAResults(const llvm::Function * /*F*/) override {
    return {this, &aliasImpl};
  }

  SVF::VersionedFlowSensitive VFS;
};

class SVFDDAAnalysis : public SVFAliasAnalysisBase {
public:
  SVFDDAAnalysis(SVF::SVFModule *Mod)
      : SVFAliasAnalysisBase(Mod, AliasAnalysisType::SVFVFS), Client(Mod) {
    Client.initialise(Mod);
    DDA.emplace(PAG, &Client);
    DDA->initialize();
    Client.answerQueries(&*DDA);
    DDA->finalize();
  }

private:
  static psr::AliasResult aliasImpl(void *AACtx, const llvm::Value *V,
                                    const llvm::Value *Rep,
                                    const llvm::DataLayout & /*DL*/) {
    auto *ModSet = SVF::LLVMModuleSet::getLLVMModuleSet();
    auto *Nod1 = ModSet->getSVFValue(V);
    auto *Nod2 = ModSet->getSVFValue(Rep);

    if (!Nod1 || !Nod2) {
      return AliasResult::MayAlias;
    }

    auto *AA = static_cast<SVFDDAAnalysis *>(AACtx);
    return translateSVFAliasResult(AA->DDA->alias(Nod1, Nod2));
  }

  FunctionAliasView doGetAAResults(const llvm::Function * /*F*/) override {
    return {this, &aliasImpl};
  }

  SVF::DDAClient Client;
  std::optional<SVF::ContextDDA> DDA;
};

} // namespace psr

static SVF::SVFModule *initSVFModule(psr::LLVMProjectIRDB &IRDB) {
  auto *Mod = SVF::LLVMModuleSet::buildSVFModule(*IRDB.getModule());
  if (!Mod) {
    throw std::runtime_error(
        "SVF failed to create an SVFModule from an llvm::Module!");
  }

  return Mod;
}

[[nodiscard]] auto psr::createSVFVFSAnalysis(LLVMProjectIRDB &IRDB)
    -> std::unique_ptr<AliasAnalysisView> {
  return std::make_unique<SVFVFSAnalysis>(initSVFModule(IRDB));
}

[[nodiscard]] auto psr::createSVFDDAAnalysis(LLVMProjectIRDB &IRDB)
    -> std::unique_ptr<AliasAnalysisView> {
  return std::make_unique<SVFDDAAnalysis>(initSVFModule(IRDB));
}
