/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_UTILS_IOFORMAT_H_
#define PHASAR_PHASARLLVM_UTILS_IOFORMAT_H_

#include <iosfwd>
#include <string>

namespace psr {

enum class IOFormat {
#define IO_FORMAT_TYPES(NAME, CMDFLAG, TYPE) TYPE,
#include "phasar/PhasarLLVM/Utils/IOFormat.def"
};

std::string to_string(const IOFormat &D);

IOFormat to_IOFormat(const std::string &S);

std::ostream &operator<<(std::ostream &os, const IOFormat &D);

} // namespace psr

#endif
