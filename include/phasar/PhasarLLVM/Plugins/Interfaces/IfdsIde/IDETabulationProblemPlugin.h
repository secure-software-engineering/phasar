/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_PLUGINS_INTERFACES_IFDSIDE_IDETABULATIONPROBLEMPLUGIN_H_
#define PHASAR_PHASARLLVM_PLUGINS_INTERFACES_IFDSIDE_IDETABULATIONPROBLEMPLUGIN_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace psr {

class LLVMBasedICFG;

class IDETabulationProblemPlugin {};

extern "C" std::unique_ptr<IDETabulationProblemPlugin>
makeIDETabulationProblemPlugin(LLVMBasedICFG &I,
                               std::vector<std::string> EntryPoints);

extern std::map<std::string,
                std::unique_ptr<IDETabulationProblemPlugin> (*)(
                    LLVMBasedICFG &I, std::vector<std::string> EntryPoints)>
    IDETabulationProblemPluginFactory;

} // namespace psr

#endif
