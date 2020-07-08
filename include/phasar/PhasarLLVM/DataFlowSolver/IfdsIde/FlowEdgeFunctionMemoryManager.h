/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWEDGEFUNCTIONMEMORYMANAGER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWEDGEFUNCTIONMEMORYMANAGER_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include <memory>
#include <functional>
#include <unordered_set>

namespace psr {

template<typename D>
class FlowEdgeFunctionMemoryManager{
    using FlowFunctionType = typename FlowFunction<D>::FlowFunctionType;
    using FlowFunctionPtrType = typename FlowFunction<D>::FlowFunctionPtrType;

    std::unordered_set<FlowMemoryFactTy> flowFunctions;

public:

    FlowFunctionType* manageFlowFunction(FlowMemoryFactTy flowfun){
        flowFunctions.insert(flowfun);
        return flowFunctions.find(flowfun)->get();
    }

};


}; // namespace psr

#endif