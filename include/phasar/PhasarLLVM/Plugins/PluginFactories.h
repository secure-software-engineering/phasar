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
#include <phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IDETabulationProblemPlugin.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IFDSTabulationProblemPlugin.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/Mono/InterMonotoneProblemPlugin.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/Mono/IntraMonotoneProblemPlugin.h>

// Inter-procedural control flow graph plug-ins
#include <phasar/PhasarLLVM/Plugins/Interfaces/ControlFlow/ICFGPlugin.h>

#endif
