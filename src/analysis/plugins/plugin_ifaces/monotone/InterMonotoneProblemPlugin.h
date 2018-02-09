/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef INTERMONOTONEPROBLEMPLUGIN_H_
#define INTERMONOTONEPROBLEMPLUGIN_H_

#include <map>
#include <memory>
#include <string>
using namespace std;

class InterMonotoneProblemPlugin {};

extern "C" unique_ptr<InterMonotoneProblemPlugin>
makeInterMonotoneProblemPlugin();

extern map<string, unique_ptr<InterMonotoneProblemPlugin> (*)()>
    InterMonotoneProblemPluginFactory;

#endif
