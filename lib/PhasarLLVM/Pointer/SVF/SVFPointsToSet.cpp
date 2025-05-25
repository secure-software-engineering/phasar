#include "phasar/PhasarLLVM/Pointer/SVF/SVFPointsToSet.h"

#include "phasar/Pointer/PointsToInfoBase.h"

#include "DDA/ContextDDA.h"
#include "DDA/DDAClient.h"
#include "InitSVF.h"
#include "MemoryModel/PointerAnalysis.h"
#include "SVF-LLVM/LLVMModule.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "WPA/Andersen.h"

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
    SVF::SymbolTableInfo::releaseSymbolInfo();
    SVF::LLVMModuleSet::releaseLLVMModuleSet();
  }

private:
  SVFPointsToSet(SVF::SVFModule *Mod)
      : IRBuilder(Mod), PAG(IRBuilder.build()) {}

  [[nodiscard]] constexpr Derived &self() noexcept {
    return static_cast<Derived &>(*this);
  }
  [[nodiscard]] constexpr const Derived &self() const noexcept {
    return static_cast<const Derived &>(*this);
  }

  [[nodiscard]] o_t
  asAbstractObjectImpl(psr::ByConstRef<v_t> Pointer) const noexcept {
    auto *ModSet = SVF::LLVMModuleSet::getLLVMModuleSet();
    auto *Nod = ModSet->getSVFValue(Pointer);

    return PAG->getValueNode(Nod);
  }

  [[nodiscard]] std::optional<v_t> asPointerOrNullImpl(o_t Obj) const noexcept {
    if (const auto *Val = PAG->getObject(Obj)->getValue()) {
      auto *ModSet = SVF::LLVMModuleSet::getLLVMModuleSet();
      if (const auto *LLVMVal = ModSet->getLLVMValue(Val)) {
        return LLVMVal;
      }
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
  VFSPointsToSetImpl(SVF::SVFModule *Mod)
      : SVFPointsToSet(Mod),
        // Note: We must use the static createVFSWPA() function, otherwise SVF
        // will leak memory
        VFS(SVF::VersionedFlowSensitive::createVFSWPA(PAG)) {}

  ~VFSPointsToSetImpl() { SVF::VersionedFlowSensitive::releaseVFSWPA(); }

  [[nodiscard]] SVF::PointerAnalysis &getPTA() const noexcept { return *VFS; }

  SVF::VersionedFlowSensitive *VFS;
};

struct DDAPointsToSetImpl : SVFPointsToSet<DDAPointsToSetImpl> {
  DDAPointsToSetImpl(SVF::SVFModule *Mod) : SVFPointsToSet(Mod), Client(Mod) {
    Client.initialise(Mod);
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
  return SVFBasedPointsToInfo(std::in_place_type<VFSPointsToSetImpl>,
                              psr::initSVFModule(IRDB));
}

auto psr::createSVFDDAPointsToInfo(LLVMProjectIRDB &IRDB)
    -> SVFBasedPointsToInfo {
  return SVFBasedPointsToInfo(std::in_place_type<DDAPointsToSetImpl>,
                              psr::initSVFModule(IRDB));
}
