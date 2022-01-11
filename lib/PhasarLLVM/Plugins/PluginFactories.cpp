/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <memory>

#include "phasar/PhasarLLVM/Plugins/PluginFactories.h"

using namespace psr;

namespace psr {

// Maps for registering the plugins
std::map<std::string, IFDSPluginConstructor> IFDSTabulationProblemPluginFactory;

std::map<std::string, IDEPluginConstructor> IDETabulationProblemPluginFactory;

std::map<std::string, IntraMonoPluginConstructor> IntraMonoProblemPluginFactory;

std::map<std::string, InterMonoPluginConstructor> InterMonoProblemPluginFactory;

std::map<std::string,
         std::unique_ptr<ICFGPlugin> (*)(
             ProjectIRDB &, const std::vector<std::string> &EntryPoints)>
    ICFGPluginFactory;

} // namespace psr
