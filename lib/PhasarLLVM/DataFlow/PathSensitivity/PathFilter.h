#ifndef PHASAR_PHASARLLVM_PATHSENSITIVITY_PATHFILTER_H
#define PHASAR_PHASARLLVM_PATHSENSITIVITY_PATHFILTER_H

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/StableVector.h"

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include <tuple>

namespace psr {

/// This could be a C++20 concept!
struct PathFilter {
  // void saveState();
  // void restoreState();
  // void saveEdge(n_t Prev, n_t Inst);
  // bool isValid() const;
  // bool saveFinalEdge(n_t Prev, n_t FinalInst);
};

template <typename... T> class PathFilterList : public std::tuple<T...> {
public:
  using n_t = const llvm::Instruction *;
  using std::tuple<T...>::tuple;

  void saveState() { saveStateImpl(std::make_index_sequence<sizeof...(T)>()); }

  void restoreState() {
    restoreStateImpl(std::make_index_sequence<sizeof...(T)>());
  }

  void saveEdge(n_t Prev, n_t Inst) {
    saveEdgeImpl(Prev, Inst, std::make_index_sequence<sizeof...(T)>());
  }

  [[nodiscard]] bool isValid() {
    return isValidImpl(std::make_index_sequence<sizeof...(T)>());
  }

  bool saveFinalEdge(n_t Prev, n_t Inst) {
    return saveFinalEdgeImpl(Prev, Inst,
                             std::make_index_sequence<sizeof...(T)>());
  }

private:
  template <size_t... I>
  void saveStateImpl(std::index_sequence<I...> /*unused*/) {
    (std::get<I>(*this).saveState(), ...);
  }
  template <size_t... I>
  void restoreStateImpl(std::index_sequence<I...> /*unused*/) {
    (std::get<I>(*this).restoreState(), ...);
  }
  template <size_t... I>
  void saveEdgeImpl(n_t Prev, n_t Inst, std::index_sequence<I...> /*unused*/) {
    (std::get<I>(*this).saveEdge(Prev, Inst), ...);
  }
  template <size_t... I>
  bool saveFinalEdgeImpl(n_t Prev, n_t Inst,
                         std::index_sequence<I...> /*unused*/) {
    return (std::get<I>(*this).saveFinalEdge(Prev, Inst) && ...);
  }
  template <size_t... I>
  bool isValidImpl(std::index_sequence<I...> /*unused*/) {
    return (std::get<I>(*this).isValid() && ...);
  }
};

template <typename... T>
static PathFilterList<T...> makePathFilterList(T &&...Filters) {
  return PathFilterList<T...>(std::forward<T>(Filters)...);
}

class CallStackPathFilter {
public:
  using n_t = const llvm::Instruction *;
  void saveState() { CallStackSafe.emplace_back(CallStackOwner.size(), TOS); }

  void restoreState() {
    auto [SaveCallStackSize, CSSave] = CallStackSafe.pop_back_val();
    assert(CallStackOwner.size() >= SaveCallStackSize);
    CallStackOwner.pop_back_n(CallStackOwner.size() - SaveCallStackSize);
    TOS = CSSave;
    Valid = true;
  }

  void saveEdge(n_t Prev, n_t Inst) {
    if (!Valid) {
      return;
    }
    if (const auto *CS = llvm::dyn_cast<llvm::CallBase>(Prev);
        CS && !isDirectSuccessorOf(Inst, CS)) {
      PHASAR_LOG_LEVEL_CAT(DEBUG, "CallStackPathFilter",
                           "Push CS: " << llvmIRToString(CS));
      pushCS(CS);

    } else if (llvm::isa<llvm::ReturnInst>(Prev) ||
               llvm::isa<llvm::ResumeInst>(Prev)) {
      /// Allow unbalanced returns
      if (!emptyCS()) {
        const auto *CS = popCS();

        PHASAR_LOG_LEVEL_CAT(DEBUG, "CallStackPathFilter",
                             "Pop CS: " << llvmIRToString(CS) << " at exit "
                                        << llvmIRToString(Prev)
                                        << " and ret-site "
                                        << llvmIRToString(Inst));

        if (!isDirectSuccessorOf(Inst, CS)) {
          /// Invalid return
          Valid = false;

          PHASAR_LOG_LEVEL_CAT(DEBUG, "CallStackPathFilter",
                               "> Invalid return");
        } else {
          PHASAR_LOG_LEVEL_CAT(DEBUG, "CallStackPathFilter", "> Valid return");
        }
      } else {
        PHASAR_LOG_LEVEL_CAT(DEBUG, "CallStackPathFilter",
                             "> Unbalanced return at exit "
                                 << llvmIRToString(Prev));
      }
      /// else: unbalanced return
    }
  }

  bool saveFinalEdge(n_t Prev, n_t FinalInst) {
    if (!Valid) {
      return false;
    }

    if (Prev && (llvm::isa<llvm::ReturnInst>(Prev) ||
                 llvm::isa<llvm::ResumeInst>(Prev))) {
      if (!emptyCS() && !isDirectSuccessorOf(FinalInst, popCS())) {
        /// Invalid return
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] bool isValid() const noexcept { return Valid; }

private:
  bool isDirectSuccessorOf(const llvm::Instruction *Succ,
                           const llvm::Instruction *Of) {

    while (const auto *Nxt = Of->getNextNode()) {
      if (Nxt == Succ) {
        return true;
      }
      Of = Nxt;

      if (Nxt->isTerminator()) {
        break;
      }
      if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(Nxt);
          Call && Call->getCalledFunction() &&
          !Call->getCalledFunction()->isDeclaration()) {
        /// Don't skip function calls. We might call the same fun twice in the
        /// same BB, so we recognize invalid paths there as well!
        return false;
      }
    }

    assert(Of->isTerminator());

    return std::find_if(llvm::succ_begin(Of), llvm::succ_end(Of),
                        [&Succ](const llvm::BasicBlock *BB) {
                          return &BB->front() == Succ;
                        }) != llvm::succ_end(Of);
  }

  void pushCS(const llvm::CallBase *CS) {
    auto *NewNode = &CallStackOwner.emplace_back();
    NewNode->Prev = TOS;
    NewNode->CS = CS;
    TOS = NewNode;
  }

  const llvm::CallBase *popCS() noexcept {
    assert(TOS && "We should already have checked for nullness...");
    const auto *Ret = TOS->CS;
    TOS = TOS->Prev;
    /// Defer the deallocation, such that we still can rollback
    return Ret;
  }

  [[nodiscard]] bool emptyCS() const noexcept { return TOS == nullptr; }

  struct CallStackNode {
    CallStackNode *Prev = nullptr;
    const llvm::CallBase *CS = nullptr;
  };
  /// Now, filter directly on the reversed DAG

  StableVector<CallStackNode> CallStackOwner;
  llvm::SmallVector<std::pair<size_t, CallStackNode *>> CallStackSafe;
  CallStackNode *TOS = nullptr;
  bool Valid = true;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_PATHSENSITIVITY_PATHFILTER_H
