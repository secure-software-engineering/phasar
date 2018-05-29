/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef IDETABULATIONPROBLEMPLUGIN_H_
#define IDETABULATIONPROBLEMPLUGIN_H_

#include <map>
#include <memory>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <string>
#include <vector>
using namespace std;

namespace psr{

class IDETabulationProblemPlugin {};

extern "C" unique_ptr<IDETabulationProblemPlugin>
makeIDETabulationProblemPlugin(LLVMBasedICFG &I, vector<string> EntryPoints);

extern map<string, unique_ptr<IDETabulationProblemPlugin> (*)(
                       LLVMBasedICFG &I, vector<string> EntryPoints)>
    IDETabulationProblemPluginFactory;

}//namespace psr

#endif
