#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/AllSanitized.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"

namespace psr::XTaint {

using l_t = AllSanitized::l_t;

EdgeFunction<l_t>
AllSanitized::compose(EdgeFunctionRef<AllSanitized> This,
                      const EdgeFunction<l_t> &SecondFunction) {
  if (llvm::isa<EdgeIdentity<l_t>>(SecondFunction)) {
    return This;
  }
  if (llvm::isa<AllBottom<l_t>>(SecondFunction)) {
    return SecondFunction;
  }
  if (llvm::isa<AllTop<l_t>>(SecondFunction)) {
    return This;
  }
  if (SecondFunction.isConstant()) {
    return SecondFunction;
  }

  auto Res = SecondFunction.computeTarget(Sanitized{});
  switch (Res.getKind()) {
  case EdgeDomain::Kind::Bot:
    // std::cerr << "Generate bot by compose sanitized" << std::endl;
    return AllBottom<l_t>{};
  case EdgeDomain::Kind::Top:
    return AllTop<l_t>{};
  case EdgeDomain::Kind::Sanitized:
    return This;
  default:
    return SecondFunction;
  }
}

EdgeFunction<l_t> AllSanitized::join(EdgeFunctionRef<AllSanitized> This,
                                     const EdgeFunction<l_t> &OtherFunction) {
  if (auto Default = defaultJoinOrNull(This, OtherFunction)) {
    return Default;
  }
  return AllBottom<l_t>{};
}

} // namespace psr::XTaint
