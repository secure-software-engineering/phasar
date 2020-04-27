#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_JOINEDGEFUNCTION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_JOINEDGEFUNCTION_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"

namespace psr {

class JoinEdgeFunction
    : public EdgeFunction<IDEGeneralizedLCA::v_t>,
      public std::enable_shared_from_this<JoinEdgeFunction> {
  std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> frst;
  std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> scnd;
  size_t maxSize;

public:
  JoinEdgeFunction(
      const std::shared_ptr<
          EdgeFunction<IDEGeneralizedLCA::v_t>> &frst,
      const std::shared_ptr<
          EdgeFunction<IDEGeneralizedLCA::v_t>> &scnd,
      size_t maxSize);
  IDEGeneralizedLCA::v_t
  computeTarget(IDEGeneralizedLCA::v_t source) override;

  std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
  composeWith(
      std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
          secondFunction) override;

  std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
  joinWith(std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
               otherFunction) override;

  bool
  equal_to(std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>>
               other) const override;
  void print(std::ostream &OS, bool isForDebug = false) const override;
  const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> &
  getFirst() const;
  const std::shared_ptr<EdgeFunction<IDEGeneralizedLCA::v_t>> &
  getSecond() const;
};

} // namespace psr

#endif
