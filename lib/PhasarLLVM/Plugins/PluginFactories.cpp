/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/Plugins/PluginFactories.h>
using namespace std;
using namespace psr;

namespace psr {

// Maps for registering the plugins
map<string, unique_ptr<IFDSTabulationProblemPlugin> (*)(
                const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
                std::set<std::string> EntryPoints)>
    IFDSTabulationProblemPluginFactory;

map<string, unique_ptr<IDETabulationProblemPlugin> (*)(
                const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
                std::set<std::string> EntryPoints)>
    IDETabulationProblemPluginFactory;

map<string, unique_ptr<IntraMonoProblemPlugin> (*)()>
    IntraMonoProblemPluginFactory;

map<string, unique_ptr<InterMonoProblemPlugin> (*)()>
    InterMonoProblemPluginFactory;

map<string,
    unique_ptr<ICFGPlugin> (*)(ProjectIRDB &, const vector<string> EntryPoints)>
    ICFGPluginFactory;

} // namespace psr
