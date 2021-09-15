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

#ifndef PHASAR_UTILS_IO_H
#define PHASAR_UTILS_IO_H

#include <filesystem>
#include <string>

namespace psr {

std::string readTextFile(const std::filesystem::path &Path);

void writeTextFile(const std::filesystem::path &Path,
                   const std::string &Content);

} // namespace psr

#endif
