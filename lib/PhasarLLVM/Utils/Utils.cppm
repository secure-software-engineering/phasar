module;

#include "phasar/PhasarLLVM/Utils/Annotation.h"
#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"
#include "phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h"
#include "phasar/PhasarLLVM/Utils/LLVMBasedContainerConfig.h"
#include "phasar/PhasarLLVM/Utils/LLVMCXXShorthands.h"
#include "phasar/PhasarLLVM/Utils/LLVMFunctionDataFlowFacts.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/PhasarLLVM/Utils/LLVMSourceManager.h"
#include "phasar/PhasarLLVM/Utils/SourceMgrPrinter.h"

export module phasar.llvm.utils;

export namespace psr {
using psr::DataFlowAnalysisType;
using psr::DefaultDominatorTreeAnalysis;
using psr::GlobalAnnotation;
using psr::toDataFlowAnalysisType;
using psr::toString;
using psr::VarAnnotation;
using psr::operator<<;
using psr::appendAllExitPoints;
using psr::computeModuleHash;
using psr::DebugLocation;
using psr::dumpIRValue;
using psr::from_json;
using psr::getAllExitPoints;
using psr::getColumnFromIR;
using psr::getDebugLocation;
using psr::getDIFileFromIR;
using psr::getDILocalVariable;
using psr::getDILocation;
using psr::getDirectoryFromIR;
using psr::getFilePathFromIR;
using psr::getFunctionArgumentNr;
using psr::getFunctionNameFromIR;
using psr::getLastInstructionOf;
using psr::getLineAndColFromIR;
using psr::getLineFromIR;
using psr::getMetaDataID;
using psr::getModuleFromVal;
using psr::getModuleIDFromIR;
using psr::getModuleNameFromVal;
using psr::getModuleSlotTrackerFor;
using psr::getNthFunctionArgument;
using psr::getNthInstruction;
using psr::getNthStoreInstruction;
using psr::getNthTermInstruction;
using psr::getSrcCodeFromIR;
using psr::getSrcCodeInfoFromIR;
using psr::getVarAnnotationIntrinsicName;
using psr::getVarNameFromIR;
using psr::getVarTypeFromIR;
using psr::globalValuesUsedinFunction;
using psr::isAllocaInstOrHeapAllocaFunction;
using psr::isGuardVariable;
using psr::isHeapAllocatingFunction;
using psr::isIntegerLikeType;
using psr::isStaticVariableLazyInitializationBranch;
using psr::isTouchVTableInst;
using psr::isVarAnnotationIntrinsic;
using psr::llvmIRToShortString;
using psr::llvmIRToStableString;
using psr::llvmIRToString;
using psr::LLVMSourceManager;
using psr::llvmTypeToString;
using psr::LLVMValueIDLess;
using psr::ManagedDebugLocation;
using psr::matchesSignature;
using psr::ModulesToSlotTracker;
using psr::Ref2PointerConverter;
using psr::SourceCodeInfo;
using psr::SourceMgrPrinter;
using psr::to_json;
} // namespace psr

export namespace psr::library_summary {
using psr::library_summary::LLVMFunctionDataFlowFacts;
using psr::library_summary::readFromFDFF;
} // namespace psr::library_summary
