/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Configuration.hh
 *
 *  Created on: 04.05.2017
 *      Author: philipp
 */

#pragma once

#include <string>

#include <boost/program_options.hpp>

namespace psr {

/// Stores the label/ tag with which we annotate the LLVM IR.
extern const std::string MetaDataKind;
/// Specifies the directory in which important configuration files are located.
extern const std::string ConfigurationDirectory;
/// Specifies the directory in which Phasar is located.
extern const std::string PhasarDirectory;
/// Name of the file storing all glibc function names.
extern const std::string GLIBCFunctionListFileName;
/// Name of the file storing all LLVM intrinsic function names.
extern const std::string LLVMIntrinsicFunctionListFileName;
/// Name of the file storing all standard header search paths used for
/// compilation.
extern const std::string HeaderSearchPathsFileName;
/// Name of the compile_commands.json file (in case we wish to rename)
extern const std::string CompileCommandsJson;
/// Variables std::map of the parsed command-line parameters
extern boost::program_options::variables_map VariablesMap;
/// Log file directory
extern const std::string LogFileDirectory;
// Variables to be used in JSON export format
/// Identifier for call graph export
extern const std::string JsonCallGraphID;
/// Identifier for type hierarchy graph export
extern const std::string JsonTypeHierarchyID;
/// Identifier for points-to graph export
extern const std::string JsonPointToGraphID;
/// Identifier for data-flow results export
extern const std::string JsonDataFlowID;

} // namespace psr
