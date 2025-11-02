#include "SVFBasedAliasAnalysis.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/AliasAnalysisView.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/PhasarLLVM/Pointer/SVF/SVFPointsToSet.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/AliasAnalysisType.h"
#include "phasar/Pointer/AliasInfoTraits.h"
#include "phasar/Pointer/AliasResult.h"
#include "phasar/Pointer/AliasSetOwner.h"
#include "phasar/Utils/AnalysisProperties.h"
#include "phasar/Utils/Fn.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "DDA/ContextDDA.h"
#include "DDA/DDAClient.h"
#include "InitSVF.h"
#include "MemoryModel/PointerAnalysis.h"
#include "PhasarSVFUtils.h"
#include "SVF-LLVM/LLVMModule.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "SVFIR/SVFIR.h"
#include "SVFIR/SVFModule.h"
#include "SVFIR/SVFType.h"
#include "Util/GeneralType.h"
#include "WPA/Andersen.h"
#include "WPA/VersionedFlowSensitive.h"

#include <memory>
#include <optional>

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

  void createAliasSet(SVF::NodeID PointerNod, AliasSetTy &Into,
                      SVF::LLVMModuleSet &ModSet) const {
    const auto &Pts = getPTA().getPts(PointerNod);
    for (auto PointeeNod : Pts) {

      if (const auto *PointeeVal =
              objectNodeToLLVMOrNull(PointeeNod, ModSet, *PAG)) {
        Into.insert(PointeeVal);
      }

      const auto &RevPts = getPTA().getRevPts(PointeeNod);
      for (auto AliasNod : RevPts) {
        if (const auto *AliasVal =
                pointerNodeToLLVMOrNull(AliasNod, ModSet, *PAG)) {
          Into.insert(AliasVal);
        }
      }
    }
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

    createAliasSet(PointerNod, *Set, *ModSet);

    return Set;
  }

  AllocationSiteSetPtrTy
  getReachableAllocationSites(const llvm::Value *Ptr, bool IntraProcOnly,
                              const llvm::Instruction * /*At*/) {
    auto Ret = std::make_unique<AliasSetTy>();
    if (!psr::isInterestingPointer(Ptr)) {
      return Ret;
    }

    auto &ModSet = *SVF::LLVMModuleSet::getLLVMModuleSet();
    auto Nod = getNodeId(Ptr, ModSet, *PAG);
    const auto &Pts = getPTA().getPts(Nod);

    const auto *VFun = AliasInfoBaseUtils::retrieveFunction(Ptr);
    const auto *VG = llvm::dyn_cast<llvm::GlobalObject>(Ptr);

    for (auto PointeeNod : Pts) {
      const auto *PointeeVal = objectNodeToLLVMOrNull(PointeeNod, ModSet, *PAG);
      if (!PointeeVal) {
        continue;
      }

      if (!IntraProcOnly || psr::isInReachableAllocationSitesTy(
                                Ptr, PointeeVal, true, VFun, VG)) {
        Ret->insert(PointeeVal);
      }
    }

    return Ret;
  }

  bool isInReachableAllocationSites(const llvm::Value *Ptr,
                                    const llvm::Value *AllocSite,
                                    bool IntraProcOnly,
                                    const llvm::Instruction * /*At*/) {

    if (IntraProcOnly &&
        !psr::isInReachableAllocationSitesTy(Ptr, AllocSite, true)) {
      return false;
    }

    auto &ModSet = *SVF::LLVMModuleSet::getLLVMModuleSet();
    auto Nod = getNodeId(Ptr, ModSet, *PAG);

    if (IntraProcOnly && llvm::isa<llvm::Argument>(AllocSite)) {
      auto AllocSiteNod = getNodeId(AllocSite, ModSet, *PAG);
      return getPTA().alias(Nod, AllocSiteNod) != SVF::NoAlias;
    }

    auto AllocSiteNod = getObjNodeId(AllocSite, ModSet, *PAG);
    const auto &Pts = getPTA().getPts(Nod);

    return Pts.test(AllocSiteNod);
  }

  void print(llvm::raw_ostream &OS) const {
    OS << "========== SVFDDAAliasSet ==========\n";

    auto &ModSet = *SVF::LLVMModuleSet::getLLVMModuleSet();
    for (const auto &[Nod, Var] : *PAG) {
      if (!Var->hasValue()) {
        continue;
      }

      const auto *PointerVal = ModSet.getLLVMValue(Var->getValue());
      if (!PointerVal) {
        continue;
      }

      AliasSetTy Buf;
      const auto &Aliases = [&, Nod = Nod]() -> const AliasSetTy & {
        auto It = Cache.find(PointerVal);
        if (It != Cache.end()) {
          return *It->second;
        }

        createAliasSet(Nod, Buf, ModSet);
        return Buf;
      }();

      if (Aliases.empty()) {
        continue;
      }

      OS << "V: " << llvmIRToString(PointerVal) << '\n';
      for (const auto *Alias : Aliases) {
        OS << "\taliases " << llvmIRToString(Alias) << '\n';
      }
    }

    OS << "=====\n";
  }

  void printAsJson(llvm::raw_ostream &OS) const {
    OS << "{\n";

    bool First = true;
    for (const auto &[Nod, Var] : *PAG) {
      if (First) {
        First = false;
      } else {
        OS << ",\n";
      }

      OS << "  \"" << Nod << "\": [";
      const auto &Pts = getPTA().getPts(Nod);
      llvm::interleaveComma(Pts, OS);
      OS << "]";
    }

    OS << "\n}\n";
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
