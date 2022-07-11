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

namespace llvm {
class raw_ostream;
}

#include <string>

namespace psr {

enum class IOFormat {
#define IO_FORMAT_TYPES(NAME, CMDFLAG, TYPE) TYPE,
#include "phasar/PhasarLLVM/Utils/IOFormat.def"
};

std::string toString(const IOFormat &D);

IOFormat toIOFormat(const std::string &S);

llvm::raw_ostream &operator<<(llvm::raw_ostream &Os, const IOFormat &D);

} // namespace psr

#endif
