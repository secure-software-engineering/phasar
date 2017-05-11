/*
 * Configuration.hh
 *
 *  Created on: 04.05.2017
 *      Author: philipp
 */

#ifndef SRC_UTILS_CONFIGURATION_HH_
#define SRC_UTILS_CONFIGURATION_HH_

#include <string>
using namespace std;

/// Stores the label/ tag with which we annotate the LLVM IR.
extern const string MetaDataKind;
/// Specifies the directory in which important configuration files are located.
extern const string ConfigurationDirectory;
/// Name of the file storing all glibc function names.
extern const string GLIBCFunctionListFileName;
/// Name of the file storing all LLVM intrinsic function names.
extern const string LLVMIntrinsicFunctionListFileName;
/// Name of the file storing all standard header search paths used for compilation.
extern const string HeaderSearchPathsFileName;

#endif /* SRC_UTILS_CONFIGURATION_HH_ */
