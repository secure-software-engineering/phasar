/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef INTRAMONOTONEPROBLEMPLUGIN_H_
#define INTRAMONOTONEPROBLEMPLUGIN_H_

#include <map>
#include <memory>
#include <string>
using namespace std;

namespace psr{

class IntraMonotoneProblemPlugin {};

extern "C" unique_ptr<IntraMonotoneProblemPlugin>
makeIntraMonotoneProblemPlugin();

extern map<string, unique_ptr<IntraMonotoneProblemPlugin> (*)()>
    IntraMonotoneProblemPluginFactory;

}//namespace psr

#endif
