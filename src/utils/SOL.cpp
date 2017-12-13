/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "SOL.h"

SOL::SOL(const string &path) : path(path) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "Loading shared object library: '" << path << "'";
  so_handle = dlopen(path.c_str(), RTLD_LAZY);
  if (!so_handle) {
    cerr << dlerror() << '\n';
    throw runtime_error("could not open shared object library: '" + path + "'");
  }
  dlerror(); // clear existing errors
}

SOL::~SOL() {
  if ((error = dlerror()) != NULL) {
    cerr << "encountered problems with shared onject library ('" + path + "'): "
         << error << '\n';
  }
  dlclose(so_handle);
}