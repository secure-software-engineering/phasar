/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IO.h
 *
 *  Created on: 02.05.2017
 *      Author: philipp
 */

#ifndef SRC_UTILS_IO_H_
#define SRC_UTILS_IO_H_

#include <boost/filesystem.hpp>
#include <fstream>
#include <string>
using namespace std;

namespace psr {

string readFile(const string &path);

void writeFile(const string &path, const string &content);

} // namespace psr

#endif /* SRC_UTILS_IO_HH_ */
