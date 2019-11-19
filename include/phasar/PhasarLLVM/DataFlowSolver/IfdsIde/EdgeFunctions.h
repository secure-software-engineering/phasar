/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * AbstractEdgeFunctions.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_H_

#include <memory>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h>
#include <set>

namespace psr {

template <typename N, typename D, typename M, typename V> class EdgeFunctions {
public:
  virtual ~EdgeFunctions() = default;
  
  
  virtual std::shared_ptr<EdgeFunction<V>>
  getNormalEdgeFunction(N curr, D currNode, N succ, D succNode) {
    if (auto Store = llvm::dyn_cast<llvm::StoreInst*>(curr)) {
      if (currNode == zerovalue && succNode == zerovalue) {
        return EdgeIdentity;
      }
      if (currnode ==zerovalue && succNode == curr.getleftoperand()) {
        struct MyEdgeFunction : EdgeFunction<int> {
          int computeTarget(int i) { return curr.getRightOperand(); }
        };
        return make_shared<MyEdgeFunction>(data);
      }
    } else if (auto Load = ...) {

    } else {
      return _Identity;
    }
  }



  virtual std::shared_ptr<EdgeFunction<V>>
  getCallEdgeFunction(N callStmt, D srcNode, M destinationMethod,
                      D destNode) = 0;
  virtual std::shared_ptr<EdgeFunction<V>>
  getReturnEdgeFunction(N callSite, M calleeMethod, N exitStmt, D exitNode,
                        N reSite, D retNode) = 0;
  virtual std::shared_ptr<EdgeFunction<V>>
  getCallToRetEdgeFunction(N callSite, D callNode, N retSite, D retSiteNode,
                           std::set<M> callees) = 0;
  virtual std::shared_ptr<EdgeFunction<V>>
  getSummaryEdgeFunction(N curr, D currNode, N succ, D succNode) = 0;
};

} // namespace psr

#endif
