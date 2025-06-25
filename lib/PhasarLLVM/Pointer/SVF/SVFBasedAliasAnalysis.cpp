#include "SVFBasedAliasAnalysis.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/AliasAnalysisView.h"
#include "phasar/PhasarLLVM/Pointer/SVF/SVFPointsToSet.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/AliasInfoTraits.h"
#include "phasar/Pointer/AliasResult.h"
#include "phasar/Pointer/AliasSetOwner.h"
#include "phasar/Utils/AnalysisProperties.h"
#include "phasar/Utils/Fn.h"

#include "llvm/IR/Value.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "DDA/ContextDDA.h"
#include "DDA/DDAClient.h"
#include "InitSVF.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "SVFIR/SVFIR.h"
#include "SVFIR/SVFModule.h"
#include "SVFIR/SVFType.h"
#include "WPA/Andersen.h"
#include "WPA/VersionedFlowSensitive.h"

#include <memory>
#include <optional>

#include <MemoryModel/PointerAnalysis.h>

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

static psr::AliasResult doAliasImpl(SVF::PointerAnalysis *AA,
                                    const llvm::Value *V,
                                    const llvm::Value *Rep) {
  auto *ModSet = SVF::LLVMModuleSet::getLLVMModuleSet();
  auto *Nod1 = ModSet->getSVFValue(V);
  auto *Nod2 = ModSet->getSVFValue(Rep);

  if (!Nod1 || !Nod2) {
    return AliasResult::MayAlias;
  }

  return translateSVFAliasResult(AA->alias(Nod1, Nod2));
}

static psr::AliasResult aliasImpl(SVF::PointerAnalysis *AA,
                                  const llvm::Value *V, const llvm::Value *Rep,
                                  const llvm::DataLayout & /*DL*/) {
  return doAliasImpl(AA, V, Rep);
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
      : SVFAliasAnalysisBase(Mod, AliasAnalysisType::SVFVFS),
        // Note: We must use the static createVFSWPA() function, otherwise SVF
        // will leak memory
        VFS(SVF::VersionedFlowSensitive::createVFSWPA(PAG)) {}

  ~SVFVFSAnalysis() override { SVF::VersionedFlowSensitive::releaseVFSWPA(); }

  [[nodiscard]] SVF::PointerAnalysis &getPTA() const { return *VFS; }
  [[nodiscard]] AliasAnalysisType getAliasAnalysisType() const noexcept {
    return AliasAnalysisType::SVFVFS;
  }
  [[nodiscard]] AnalysisProperties getAnalysisProperties() const noexcept {
    return AnalysisProperties::FlowSensitive;
  }

private:
  FunctionAliasView doGetAAResults(const llvm::Function * /*F*/) override {
    return {VFS, fn<aliasImpl>};
  }

  SVF::VersionedFlowSensitive *VFS;
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

  [[nodiscard]] SVF::PointerAnalysis &getPTA() const { return *DDA; }
  [[nodiscard]] AliasAnalysisType getAliasAnalysisType() const noexcept {
    return AliasAnalysisType::SVFDDA;
  }
  [[nodiscard]] AnalysisProperties getAnalysisProperties() const noexcept {
    return AnalysisProperties::ContextSensitive;
  }

private:
  FunctionAliasView doGetAAResults(const llvm::Function * /*F*/) override {
    return {&*DDA, fn<aliasImpl>};
  }

  SVF::DDAClient Client;
  // Note: SVF is not thread-safe anyway, so this 'mutable' should not be a
  // problem
  mutable std::optional<SVF::ContextDDA> DDA;
};

} // namespace psr

auto psr::createSVFVFSAnalysis(LLVMProjectIRDB &IRDB)
    -> std::unique_ptr<AliasAnalysisView> {

  return std::make_unique<SVFVFSAnalysis>(psr::initSVFModule(IRDB));
}

auto psr::createSVFDDAAnalysis(LLVMProjectIRDB &IRDB)
    -> std::unique_ptr<AliasAnalysisView> {

  return std::make_unique<SVFDDAAnalysis>(psr::initSVFModule(IRDB));
}

namespace psr {

class SVFAliasInfoImpl;

template <>
struct AliasInfoTraits<SVFAliasInfoImpl>
    : DefaultAATraits<const llvm::Value *, const llvm::Instruction *> {};

class SVFAliasInfoImpl
    : public SVFDDAAnalysis,
      public AnalysisPropertiesMixin<SVFAliasInfoImpl>,
      public DefaultAATraits<const llvm::Value *, const llvm::Instruction *> {
public:
  using SVFDDAAnalysis::SVFDDAAnalysis;

  [[nodiscard]] bool isInterProcedural() const noexcept { return true; }

  [[nodiscard]] psr::AliasResult alias(const llvm::Value *V,
                                       const llvm::Value *Rep,
                                       const llvm::Instruction * /*At*/) const {
    return doAliasImpl(&getPTA(), V, Rep);
  }

  [[nodiscard]] AliasSetPtrTy getAliasSet(const llvm::Value *Ptr,
                                          const llvm::Instruction * /*At*/) {
    auto &Ret = Cache[Ptr];
    if (Ret) {
      return Ret;
    }

    auto Set = Owner.acquire();
    Ret = Set;

    auto *ModSet = SVF::LLVMModuleSet::getLLVMModuleSet();
    auto *Nod = ModSet->getSVFValue(Ptr);

    auto PointerNod = PAG->getValueNode(Nod);

    const auto &Pts = getPTA().getPts(PointerNod);
    for (auto PointeeNod : Pts) {

      if (const SVF::MemObj *Mem = PAG->getObject(PointeeNod)) {
        if (const auto *Val = Mem->getValue()) {
          if (const auto *LLVMVal = ModSet->getLLVMValue(Val)) {
            Set->insert(LLVMVal);
          }
        }
      }

      const auto &RevPts = getPTA().getRevPts(PointeeNod);
      for (auto AliasNod : RevPts) {
        const auto *AliasGNod = PAG->getGNode(AliasNod);
        if (!AliasGNod) {
          continue;
        }
        const auto *AliasVal = AliasGNod->getValue();
        if (!AliasVal) {
          continue;
        }
        if (const auto *LLVMAliasVal = ModSet->getLLVMValue(AliasVal)) {
          Set->insert(LLVMAliasVal);
        }
      }
    }

    return Set;
  }

  // TODO: reachable allocation sites without too much code duplication

  AllocationSiteSetPtrTy
  getReachableAllocationSites(const llvm::Value *Ptr, bool IntraProcOnly,
                              const llvm::Instruction *At) {
    llvm::report_fatal_error(
        "[getReachableAllocationSites]: Not implemented yet!");
  }

  bool isInReachableAllocationSites(const llvm::Value *Ptr,
                                    const llvm::Value *AllocSite,
                                    bool IntraProcOnly,
                                    const llvm::Instruction *At) {
    llvm::report_fatal_error(
        "[getReachableAllocationSites]: Not implemented yet!");
  }

  void print(llvm::raw_ostream &OS) const {
    // TODO
  }

  void printAsJson(llvm::raw_ostream &OS) const {
    // TODO
  }

  void mergeWith(SVFAliasInfoImpl & /*Other*/) {
    llvm::report_fatal_error("[mergeWith]: not supported");
  }

  void introduceAlias(const llvm::Value * /*V1*/, const llvm::Value * /*V2*/,
                      const llvm::Instruction * /*At*/, AliasResult /*Kind*/) {
    llvm::report_fatal_error("[introduceAlias]: not supported");
  }

private:
  llvm::DenseMap<const llvm::Value *, AliasSetPtrTy> Cache;
  AliasSetOwner<AliasSetTy>::memory_resource_type MRes;
  AliasSetOwner<AliasSetTy> Owner{&MRes};
};
} // namespace psr

auto psr::createLLVMSVFDDAAliasInfo(LLVMProjectIRDB &IRDB) -> LLVMAliasInfo {
  return std::make_unique<SVFAliasInfoImpl>(psr::initSVFModule(IRDB));
}
