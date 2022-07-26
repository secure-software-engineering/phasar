/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Hashing.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/ComposeEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/DebugEdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/GenEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/Helpers.h"
#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"

namespace psr::XTaint {

EdgeFunction<EdgeDomain>::EdgeFunctionPtrType
getEdgeIdentity([[maybe_unused]] const llvm::Instruction *Inst) {
  return EdgeIdentity<EdgeDomain>::getInstance();
}

bool isEdgeIdentity(EdgeFunction<EdgeDomain> *EF) {
  return EF == &*getEdgeIdentity(nullptr);
}

EdgeFunction<EdgeDomain>::EdgeFunctionPtrType
getGenEdgeFunction(BasicBlockOrdering &BBO) {
  static llvm::SmallDenseMap<BasicBlockOrdering *,
                             EdgeFunction<EdgeDomain>::EdgeFunctionPtrType, 2>
      Cache;

  auto &Ret = Cache[&BBO];
  if (!Ret) {
    Ret = std::make_shared<GenEdgeFunction>(BBO, nullptr);
  }
  return Ret;
}

llvm::hash_code
getHashCode(const EdgeFunction<EdgeDomain>::EdgeFunctionPtrType &EF) {
  if (auto *XEF = dynamic_cast<EdgeFunctionBase *>(&*EF)) {
    return XEF->getHashCode();
  }
  return llvm::hash_value(&*EF);
}

EdgeFunction<EdgeDomain>::EdgeFunctionPtrType getAllTop() {
  static AllTop<EdgeDomain>::EdgeFunctionPtrType TopEF =
      makeEF<AllTop<EdgeDomain>>(Top{});
  return TopEF;
}

EdgeFunction<EdgeDomain>::EdgeFunctionPtrType getAllBot() {
  static AllBottom<EdgeDomain>::EdgeFunctionPtrType BotEF =
      makeEF<AllBottom<EdgeDomain>>(Bottom{});
  return BotEF;
}

EdgeFunction<EdgeDomain>::EdgeFunctionPtrType getAllSanitized() {
  struct AllSanitized final
      : public EdgeFunction<EdgeDomain>,
        public std::enable_shared_from_this<AllSanitized> {

    using l_t = EdgeDomain;

    l_t computeTarget([[maybe_unused]] l_t Source) override {
      return Sanitized{};
    }

    EdgeFunctionPtrType
    composeWith(EdgeFunctionPtrType SecondFunction) override {
      if (dynamic_cast<EdgeIdentity<l_t> *>(&*SecondFunction)) {
        return shared_from_this();
      }
      if (dynamic_cast<AllBottom<l_t> *>(&*SecondFunction)) {
        return SecondFunction;
      }
      if (dynamic_cast<AllTop<l_t> *>(&*SecondFunction)) {
        return shared_from_this();
      }
      if (dynamic_cast<GenEdgeFunction *>(&*SecondFunction)) {
        return SecondFunction;
      }

      auto Res = SecondFunction->computeTarget(Sanitized{});
      switch (Res.getKind()) {
      case EdgeDomain::Kind::Bot:
        // std::cerr << "Generate bot by compose sanitized" << std::endl;
        return getAllBot();
      case EdgeDomain::Kind::Top:
        return getAllTop();
      case EdgeDomain::Kind::Sanitized:
        return shared_from_this();
      default:
        return SecondFunction;
      }
    }

    EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction) override {
      if (dynamic_cast<psr::AllBottom<l_t> *>(&*OtherFunction)) {
        return shared_from_this();
      }

      return OtherFunction;
    }

    bool equal_to(EdgeFunctionPtrType OtherFunction) const override {
      return &*OtherFunction == this;
    }

    void print(llvm::raw_ostream &OS,
               [[maybe_unused]] bool IsForDebug = false) const override {
      OS << "SanitizedEF";
    }
  };

  static AllSanitized::EdgeFunctionPtrType SaniEF = makeEF<AllSanitized>();
  return SaniEF;
}
} // namespace psr::XTaint
