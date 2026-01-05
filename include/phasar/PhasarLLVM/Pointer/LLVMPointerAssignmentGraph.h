#pragma once

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/Pointer/PointerAssignmentGraph.h"

namespace llvm {
class Instruction;
class Value;
class Function;
} // namespace llvm

namespace psr {

/// The PAG-node used for LLVM-based pointer-assignment graphs. It consists of
/// regular LLVM values + special return-slots per non-void function. In that
/// case, the stored pointer is the corresponding function.
///
/// This class is compatible with llvm::TinyPtrVector and other APIs relying on
/// llvm::PointerLikeTypeTraits.
struct PAGVariable : public llvm::PointerIntPair<const llvm::Value *, 1, bool> {
  using Base = llvm::PointerIntPair<const llvm::Value *, 1, bool>;

  struct Return {
    const llvm::Function *Fun;
  };

  PAGVariable() noexcept = default; // To enable move-assign in TinyPtrVector
  PAGVariable(const llvm::Value *Var) noexcept : Base(Var, false) {}
  PAGVariable(Return RetVar) noexcept : Base(RetVar.Fun, true) {
    assert(RetVar.Fun != nullptr);
  }

  [[nodiscard]] bool isReturnVariable() const noexcept {
    return this->getInt();
  }

  [[nodiscard]] bool isInFunction() const noexcept {
    return isReturnVariable() ||
           llvm::isa<llvm::Argument, llvm::Instruction>(this->getPointer());
  }

  [[nodiscard]] const llvm::Function *getFunction() const noexcept {
    const auto *Ptr = this->getPointer();
    if (isReturnVariable()) {
      return llvm::cast<llvm::Function>(Ptr);
    }
    if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(Ptr)) {
      return Arg->getParent();
    }
    if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(Ptr)) {
      return Inst->getFunction();
    }

    return nullptr;
  }

  [[nodiscard]] const llvm::Value *get() const noexcept {
    return this->getPointer();
  }
  explicit operator const llvm::Value *() const noexcept { return get(); }

  [[nodiscard]] const llvm::Value *valueOrNull() const noexcept {
    return isReturnVariable() ? nullptr : get();
  }

  friend bool operator==(PAGVariable V1, PAGVariable V2) noexcept {
    return V1.getOpaqueValue() == V2.getOpaqueValue();
  }
  friend bool operator!=(PAGVariable V1, PAGVariable V2) noexcept {
    return !(V1 == V2);
  }

  friend auto hash_value(PAGVariable V) noexcept {
    return llvm::hash_value(V.getOpaqueValue());
  }
};

[[nodiscard]] std::string to_string(PAGVariable Var);

struct LLVMPAGDomain : LLVMAnalysisDomainDefault {
  using v_t = PAGVariable;
};

class LLVMPAGBuilder : public PAGBuilder<LLVMPAGDomain> {
public:
  void buildPAG(const LLVMProjectIRDB &IRDB, ValueCompressor<v_t> &VC,
                pag::PBStrategyRef<LLVMPAGDomain> Strategy) override;

private:
  struct PAGBuildData;
};

} // namespace psr

namespace llvm {
template <> struct PointerLikeTypeTraits<psr::PAGVariable> {
  static void *getAsVoidPointer(psr::PAGVariable P) {
    return P.getOpaqueValue();
  }

  static psr::PAGVariable getFromVoidPointer(void *P) {
    psr::PAGVariable V{nullptr};
    static_cast<psr::PAGVariable::Base &>(V) =
        psr::PAGVariable::Base::getFromOpaqueValue(P);
    return V;
  }

  static psr::PAGVariable getFromVoidPointer(const void *P) {
    psr::PAGVariable V{nullptr};
    static_cast<psr::PAGVariable::Base &>(V) =
        psr::PAGVariable::Base::getFromOpaqueValue(P);
    return V;
  }

  static constexpr int NumLowBitsAvailable =
      PointerLikeTypeTraits<psr::PAGVariable::Base>::NumLowBitsAvailable;
};

template <> struct DenseMapInfo<psr::PAGVariable> {
  static psr::PAGVariable getEmptyKey() noexcept {
    return DenseMapInfo<const llvm::Value *>::getEmptyKey();
  }
  static psr::PAGVariable getTombstoneKey() noexcept {
    return DenseMapInfo<const llvm::Value *>::getTombstoneKey();
  }
  static hash_code getHashValue(psr::PAGVariable V) noexcept {
    return hash_value(V);
  }

  static bool isEqual(psr::PAGVariable V1, psr::PAGVariable V2) noexcept {
    return V1 == V2;
  }
};
} // namespace llvm
