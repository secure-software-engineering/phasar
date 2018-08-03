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

#ifndef PHASAR_UTILS_IO_H_
#define PHASAR_UTILS_IO_H_

#include <string>

namespace psr {

std::string readFile(const std::string &path);

void writeFile(const std::string &path, const std::string &content);

} // namespace psr

#endif
