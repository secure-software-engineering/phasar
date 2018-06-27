/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#pragma once

#include <map>
#include <memory>
#include <string>

namespace psr {

class InterMonotoneProblemPlugin {};

extern "C" std::unique_ptr<InterMonotoneProblemPlugin>
makeInterMonotoneProblemPlugin();

extern std::map<std::string, std::unique_ptr<InterMonotoneProblemPlugin> (*)()>
    InterMonotoneProblemPluginFactory;

} // namespace psr
