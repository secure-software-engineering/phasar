/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_PLUGINS_INTERFACES_MONO_INTERMONOPROBLEMPLUGIN_H_
#define PHASAR_PHASARLLVM_PLUGINS_INTERFACES_MONO_INTERMONOPROBLEMPLUGIN_H_

#include <map>
#include <memory>
#include <string>

namespace psr {

class InterMonoProblemPlugin {};

extern "C" std::unique_ptr<InterMonoProblemPlugin> makeInterMonoProblemPlugin();

extern std::map<std::string, std::unique_ptr<InterMonoProblemPlugin> (*)()>
    InterMonoProblemPluginFactory;

} // namespace psr

#endif
