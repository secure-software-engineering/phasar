/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Plugins/PluginFactories.h"

using namespace std;
using namespace psr;

namespace psr {

// Maps for registering the plugins
map<string, IFDSPluginConstructor> IFDSTabulationProblemPluginFactory;

map<string, IDEPluginConstructor> IDETabulationProblemPluginFactory;

map<string, IntraMonoPluginConstructor> IntraMonoProblemPluginFactory;

map<string, InterMonoPluginConstructor> InterMonoProblemPluginFactory;

map<string, unique_ptr<ICFGPlugin> (*)(ProjectIRDB &,
                                       const vector<string> &EntryPoints)>
    ICFGPluginFactory;

} // namespace psr
