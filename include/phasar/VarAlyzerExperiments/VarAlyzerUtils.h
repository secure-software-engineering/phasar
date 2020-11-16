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

#include <set>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"

namespace psr {

class ProjectIRDB;
class LLVMBasedICFG;

bool isValidLLVMIRFile(const boost::filesystem::path &FilePath);

std::vector<std::string> makeStringVectorFromPathVector(
    const std::vector<boost::filesystem::path> &Paths);

enum class OpenSSLEVPAnalysisType { CIPHER, MAC, MD };

OpenSSLEVPAnalysisType to_OpenSSLEVPAnalysisType(const std::string &Str);

std::set<std::string> getEntryPointsForCallersOf(const std::string &FunName,
                                                 ProjectIRDB &IR,
                                                 LLVMBasedICFG &ICF);

std::set<std::string>
getEntryPointsForCallersOfDesugared(const std::string &FunName, ProjectIRDB &IR,
                                    LLVMBasedICFG &ICF);

} // namespace psr

#endif
