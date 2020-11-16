/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_VARALYZEREXPERIMENTS_VARALYZERUTILS_H_
#define PHASAR_VARALYZEREXPERIMENTS_VARALYZERUTILS_H_

#include <llvm/ADT/DenseMapInfo.h>
#include <llvm/ADT/Hashing.h>
#include <llvm/ADT/StringRef.h>
#include <set>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"

namespace psr {

bool isValidLLVMIRFile(const boost::filesystem::path &FilePath);

std::vector<std::string> makeStringVectorFromPathVector(
    const std::vector<boost::filesystem::path> &Paths);

enum class OpenSSLEVPAnalysisType { ALL, CIPHER, MAC, MD };

OpenSSLEVPAnalysisType to_OpenSSLEVPAnalysisType(const std::string &Str);

std::set<std::string> getEntryPointsForCallersOf();


} // namespace psr


#endif
