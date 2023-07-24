/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * CFG.h
 *
 *  Created on: 07.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_CONTROLFLOW_SPECIALMEMBERFUNCTIONTYPE_H
#define PHASAR_CONTROLFLOW_SPECIALMEMBERFUNCTIONTYPE_H

#include "llvm/ADT/StringRef.h"

#include <string>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace psr {

enum class SpecialMemberFunctionType {
#define SPECIAL_MEMBER_FUNCTION_TYPES(NAME, TYPE) TYPE,
#include "phasar/ControlFlow/SpecialMemberFunctionType.def"
};

std::string toString(SpecialMemberFunctionType SMFT);

SpecialMemberFunctionType toSpecialMemberFunctionType(llvm::StringRef SMFT);

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              SpecialMemberFunctionType SMFT);
} // namespace psr

#endif // PHASAR_CONTROLFLOW_SPECIALMEMBERFUNCTIONTYPE_H
