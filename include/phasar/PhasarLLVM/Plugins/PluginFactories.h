/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_PLUGINS_PLUGINFACTORIES_H_
#define PHASAR_PHASARLLVM_PLUGINS_PLUGINFACTORIES_H_

// Problem plug-ins
#include "phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IDETabulationProblemPlugin.h"
#include "phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IFDSTabulationProblemPlugin.h"
#include "phasar/PhasarLLVM/Plugins/Interfaces/Mono/InterMonoProblemPlugin.h"
#include "phasar/PhasarLLVM/Plugins/Interfaces/Mono/IntraMonoProblemPlugin.h"

// Inter-procedural control flow graph plug-ins
#include "phasar/PhasarLLVM/Plugins/Interfaces/ControlFlow/ICFGPlugin.h"

namespace psr {

using IFDSPluginConstructor = std::unique_ptr<IFDSTabulationProblemPlugin> (*)(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints);

using IDEPluginConstructor = std::unique_ptr<IDETabulationProblemPlugin> (*)(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    std::set<std::string> EntryPoints);

using IntraMonoPluginConstructor = std::unique_ptr<IntraMonoProblemPlugin> (*)(
    const ProjectIRDB *IRDB,
    const TypeHierarchy<IntraMonoProblemPlugin::t_t,
                        IntraMonoProblemPlugin::f_t> *TH,
    const IntraMonoProblemPlugin::i_t *CF,
    const PointsToInfo<IntraMonoProblemPlugin::v_t, IntraMonoProblemPlugin::n_t>
        *PT,
    std::set<std::string> EntryPoints);

using InterMonoPluginConstructor = std::unique_ptr<InterMonoProblemPlugin> (*)(
    const ProjectIRDB *IRDB,
    const TypeHierarchy<InterMonoProblemPlugin::t_t,
                        InterMonoProblemPlugin::f_t> *TH,
    const InterMonoProblemPlugin::i_t *ICF,
    const PointsToInfo<InterMonoProblemPlugin::v_t, InterMonoProblemPlugin::n_t>
        *PT,
    std::set<std::string> EntryPoints);

} // namespace psr

#endif
