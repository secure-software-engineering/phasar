#include "phasar/PhasarLLVM/Pointer/SVF/SVFPointsToSet.h"

#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/Pointer/PointsToInfoBase.h"

#include "llvm/IR/Instruction.h"
#include "llvm/Support/ErrorHandling.h"

#include "DDA/ContextDDA.h"
#include "DDA/DDAClient.h"
#include "InitSVF.h"
#include "MemoryModel/PointerAnalysis.h"
#include "PhasarSVFUtils.h"
#include "SVF-LLVM/LLVMModule.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "WPA/Andersen.h"

#include <memory>
#include <utility>

namespace {
template <typename Derived> class SVFPointsToSet;
struct DDAPointsToSetImpl;
struct VFSPointsToSetImpl;
} // namespace

namespace psr {
template <typename Derived>
struct PointsToTraits<SVFPointsToSet<Derived>> : SVFPointsToInfoTraits {};
template <>
struct PointsToTraits<DDAPointsToSetImpl> : SVFPointsToInfoTraits {};
template <>
struct PointsToTraits<VFSPointsToSetImpl> : SVFPointsToInfoTraits {};
} // namespace psr

namespace {

template <typename Derived>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class SVFPointsToSet : public psr::PointsToInfoBase<SVFPointsToSet<Derived>> {
  using base_t = psr::PointsToInfoBase<SVFPointsToSet<Derived>>;
  friend base_t;

public:
  using typename base_t::n_t;
  using typename base_t::o_t;
  using typename base_t::PointsToSetPtrTy;
  using typename base_t::PointsToSetTy;
  using typename base_t::v_t;

  ~SVFPointsToSet() {
    SVF::SVFIR::releaseSVFIR();
    SVF::AndersenWaveDiff::releaseAndersenWaveDiff();
    SVF::LLVMModuleSet::releaseLLVMModuleSet();
  }

  [[nodiscard]] constexpr SVF::SVFIR &getPAG() const noexcept { return *PAG; }

private:
  SVFPointsToSet() : PAG(IRBuilder.build()) {}

  [[nodiscard]] constexpr Derived &self() noexcept {
    return static_cast<Derived &>(*this);
  }
  [[nodiscard]] constexpr const Derived &self() const noexcept {
    return static_cast<const Derived &>(*this);
  }

  [[nodiscard]] o_t
  asAbstractObjectImpl(psr::ByConstRef<v_t> Pointer) const noexcept {
    auto *ModSet = SVF::LLVMModuleSet::getLLVMModuleSet();
    return psr::getNodeId(Pointer, *ModSet);
  }

  [[nodiscard]] std::optional<v_t> asPointerOrNullImpl(o_t Obj) const noexcept {
    if (const auto *LLVMVal = psr::objectNodeToLLVMOrNull(
            Obj, *SVF::LLVMModuleSet::getLLVMModuleSet(), *PAG)) {
      return LLVMVal;
    }

    return std::nullopt;
  }

  bool mayPointsToImpl(o_t Pointer, o_t Obj, n_t /*AtInstruction*/) const {
    auto &PTA = self().getPTA();
    const auto &Pts = PTA.getPts(Pointer);
    return Pts.test(Obj);
  }

  using base_t::mayPointsToImpl;

  PointsToSetPtrTy getPointsToSetImpl(o_t Pointer,
                                      n_t /*AtInstruction*/) const {
    auto &PTA = self().getPTA();
    const auto &Pts = PTA.getPts(Pointer);

    // TODO: Should we cache this?
    PointsToSetPtrTy Ret;
    Ret.reserve(Pts.count());
    Ret.insert(Pts.begin(), Pts.end());
    return Ret;
  }

  using base_t::getPointsToSetImpl;

protected:
  SVF::SVFIRBuilder IRBuilder;
  SVF::SVFIR *PAG;
  friend Derived;
};

struct VFSPointsToSetImpl : SVFPointsToSet<VFSPointsToSetImpl> {
  VFSPointsToSetImpl()
      : // Note: We must use the static createVFSWPA() function, otherwise SVF
        // will leak memory
        VFS(SVF::VersionedFlowSensitive::createVFSWPA(PAG)) {}

  ~VFSPointsToSetImpl() { SVF::VersionedFlowSensitive::releaseVFSWPA(); }

  [[nodiscard]] SVF::PointerAnalysis &getPTA() const noexcept { return *VFS; }

  SVF::VersionedFlowSensitive *VFS;
};

struct DDAPointsToSetImpl : SVFPointsToSet<DDAPointsToSetImpl> {
  DDAPointsToSetImpl() {
    Client.initialise();
    DDA.emplace(PAG, &Client);
    DDA->initialize();
    Client.answerQueries(&*DDA);
    DDA->finalize();
  }

  [[nodiscard]] SVF::PointerAnalysis &getPTA() const noexcept { return *DDA; }

  SVF::DDAClient Client;

  // Note: SVF is not thread-safe anyway, so this 'mutable' should not be a
  // problem
  mutable std::optional<SVF::ContextDDA> DDA;
};
} // namespace

auto psr::createSVFVFSPointsToInfo(LLVMProjectIRDB &IRDB)
    -> SVFBasedPointsToInfo {
  psr::initSVFModule(IRDB);
  return SVFBasedPointsToInfo(std::in_place_type<VFSPointsToSetImpl>);
}

auto psr::createSVFDDAPointsToInfo(LLVMProjectIRDB &IRDB)
    -> SVFBasedPointsToInfo {
  psr::initSVFModule(IRDB);
  return SVFBasedPointsToInfo(std::in_place_type<DDAPointsToSetImpl>);
}

auto psr::createSVFPointsToInfo(LLVMProjectIRDB &IRDB,
                                SVFPointsToAnalysisType PTATy)
    -> SVFBasedPointsToInfo {
  psr::initSVFModule(IRDB);
  switch (PTATy) {
  case SVFPointsToAnalysisType::DDA:
    return SVFBasedPointsToInfo(std::in_place_type<DDAPointsToSetImpl>);
  case SVFPointsToAnalysisType::VFS:
    return SVFBasedPointsToInfo(std::in_place_type<VFSPointsToSetImpl>);
  }
  llvm_unreachable("Should have handled all SVFPointsToAnalysisType variants "
                   "in the switch above!");
}

namespace {

template <typename SVFPointsToSetT> struct SVFLLVMPointsToIterator {
  using n_t = const llvm::Instruction *;
  using v_t = const llvm::Value *;
  using o_t = const llvm::Value *;

  SVFLLVMPointsToIterator() = default;

  [[nodiscard]] constexpr o_t asAbstractObject(v_t Pointer) const noexcept {
    return Pointer;
  }

  [[nodiscard]] SVF::NodeID getNodeId(v_t Pointer) const noexcept {
    auto *ModSet = SVF::LLVMModuleSet::getLLVMModuleSet();
    return psr::getNodeId(Pointer, *ModSet);
  }
  [[nodiscard]] SVF::NodeID getObjNodeId(o_t Obj) const noexcept {
    auto *ModSet = SVF::LLVMModuleSet::getLLVMModuleSet();
    return psr::getObjNodeId(Obj, *ModSet);
  }

  void forallPointeesOf(o_t Pointer, n_t /*At*/,
                        llvm::function_ref<void(o_t)> WithPointee) const {
    SVF::PointerAnalysis &PTA = PT.getPTA();

    auto Nod = getNodeId(Pointer);

    const auto &Pts = PTA.getPts(Nod);

    auto *ModSet = SVF::LLVMModuleSet::getLLVMModuleSet();
    SVF::SVFIR &PAG = PT.getPAG();
    for (auto PointeeNod : Pts) {
      if (const auto *PointeeVal =
              psr::objectNodeToLLVMOrNull(PointeeNod, *ModSet, PAG)) {
        WithPointee(PointeeVal);
      }
    }
  }

  [[nodiscard]] bool mayPointsTo(o_t Pointer, o_t Obj, n_t /*At*/) const {
    SVF::PointerAnalysis &PTA = PT.getPTA();
    auto PointerNod = getNodeId(Pointer);
    auto ObjNod = getObjNodeId(Obj);

    const auto &Pts = PTA.getPts(PointerNod);
    return Pts.test(ObjNod);
  }

  SVFPointsToSetT PT;
};
} // namespace

auto psr::createLLVMSVFPointsToIterator(LLVMProjectIRDB &IRDB,
                                        SVFPointsToAnalysisType PTATy)
    -> LLVMPointsToIterator {
  psr::initSVFModule(IRDB);

  switch (PTATy) {
  case SVFPointsToAnalysisType::DDA:
    return {std::make_unique<SVFLLVMPointsToIterator<DDAPointsToSetImpl>>()};
  case SVFPointsToAnalysisType::VFS:
    return {std::make_unique<SVFLLVMPointsToIterator<VFSPointsToSetImpl>>()};
  }
  llvm_unreachable("Should have handled all SVFPointsToAnalysisType variants "
                   "in the switch above!");
}
