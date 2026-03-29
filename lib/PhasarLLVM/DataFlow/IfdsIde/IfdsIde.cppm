module;

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultAliasAwareIDEProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocation.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocationFactory.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/AllSanitized.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/ComposeEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/EdgeDomain.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/GenEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/Helpers.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/KillIfSanitizedEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/TransferEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintAnalysisBase.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintEdgeFunctionBase.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEExtendedTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEFeatureTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/BinaryEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/ConstantHelper.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/IDEGeneralizedLCA.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/LCAEdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/MapFactsToCalleeFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/MapFactsToCallerFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEGeneralizedLCA/TypecastEdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEInstInteractionAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEProtoAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDESecureHeapPropagation.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDESolverTest.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDETypeStateAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSConstAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSProtoAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSSignAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSSolverTest.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSTypeAnalysis.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSUninitializedVariables.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/CSTDFILEIOTypeStateDescription.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLEVPKDFCTXDescription.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLSecureHeapDescription.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/TypeStateDescriptions/OpenSSLSecureMemoryDescription.h"

export module phasar.llvm.dataflow.ifdside;

export namespace psr {
using psr::AbstractMemoryLocation;
using psr::AbstractMemoryLocationFactory;
using psr::BasicBlockOrdering;
using psr::DToString;
using psr::FeatureTaintGenerator;
using psr::hash_value;
using psr::IDEExtendedTaintAnalysis;
using psr::IDEExtendedTaintAnalysisDomain;
using psr::IDEFeatureTaintAnalysis;
using psr::IDEFeatureTaintAnalysisDomain;
using psr::IDEFeatureTaintEdgeFact;
using psr::IDEIIAFlowFact;
using psr::IDELinearConstantAnalysis;
using psr::IFDSSolverTest;
using psr::JoinLatticeTraits;
using psr::LToString;
using psr::NonTopBotValue;
using psr::operator<<;
using psr::DefaultAliasAwareIDEProblem;
using psr::DefaultAliasAwareIFDSProblem;
using psr::DefaultNoAliasIDEProblem;
using psr::DefaultNoAliasIFDSProblem;
using psr::DToString;
using psr::IDEExtendedTaintAnalysis;
using psr::IDEInstInteractionAnalysis;
using psr::IDEInstInteractionAnalysisDomain;
using psr::IDEInstInteractionAnalysisT;
using psr::IDELinearConstantAnalysis;
using psr::IDELinearConstantAnalysisDomain;
using psr::IDEProtoAnalysis;
using psr::IDEProtoAnalysisDomain;
using psr::IDESecureHeapPropagation;
using psr::IDESecureHeapPropagationAnalysisDomain;
using psr::IDESolverTest;
using psr::IDESolverTestAnalysisDomain;
using psr::IDETypeStateAnalysis;
using psr::IDETypeStateAnalysisDomain;
using psr::IFDSConstAnalysis;
using psr::IFDSProtoAnalysis;
using psr::IFDSSignAnalysis;
using psr::IFDSSolverTest;
using psr::IFDSTaintAnalysis;
using psr::IFDSTypeAnalysis;
using psr::IFDSUninitializedVariables;
using psr::JoinLatticeTraits;
using psr::LLVMZeroValue;
using psr::LToString;
using psr::mapFactsAlongsideCallSite;
using psr::mapFactsToCallee;
using psr::mapFactsToCaller;
using psr::SecureHeapFact;
using psr::SecureHeapValue;
using psr::strongUpdateStore;
using psr::operator<<;
using psr::operator==;
using psr::CSTDFILEIOState;
using psr::EdgeFunctionBase;
using psr::IDEGeneralizedLCA;
using psr::IDEGeneralizedLCADomain;
using psr::JoinLatticeTraits;
using psr::to_string;
using psr::operator<<;
using psr::CSTDFILEIOTypeStateDescription;
using psr::IDETypeStateAnalysis;
using psr::OpenSSLEVPKDFCTXDescription;
using psr::OpenSSLEVPKDFCTXState;
using psr::OpenSSLEVPKDFDescription;
using psr::OpenSSLEVPKDFState;
using psr::OpenSSLSecureHeapDescription;
using psr::OpenSSLSecureHeapState;
using psr::OpenSSLSecureMemoryDescription;
using psr::OpenSSLSecureMemoryState;
using psr::to_string;
using psr::TypeStateDescription;
using psr::TypeStateDescriptionBase;
} // namespace psr

export namespace std {
using std::hash;
} // namespace std

export namespace llvm {
using llvm::DenseMapInfo;
} // namespace llvm

export namespace psr::XTaint {
using psr::XTaint::AnalysisBase;
using psr::XTaint::ComposeEdgeFunction;
using psr::XTaint::EdgeDomain;
using psr::XTaint::KillIfSanitizedEdgeFunction;
using psr::XTaint::makeComposeEF;
using psr::XTaint::makeFF;
using psr::XTaint::Sanitized;
using psr::XTaint::TransferEdgeFunction;
} // namespace psr::XTaint

namespace psr::glca {
using psr::glca::BinaryEdgeFunction;
using psr::glca::compare;
using psr::glca::EdgeValue;
using psr::glca::EdgeValueSet;
using psr::glca::isConstant;
using psr::glca::join;
using psr::glca::Ordering;
using psr::glca::performBinOp;
using psr::glca::performTypecast;
using psr::glca::operator<;
using psr::glca::isTopValue;
using psr::glca::operator<<;
using psr::glca::EdgeValueSet;
using psr::glca::LCAEdgeFunctionComposer;
using psr::glca::MapFactsToCalleeFlowFunction;
using psr::glca::MapFactsToCallerFlowFunction;
using psr::glca::TypecastEdgeFunction;
using psr::glca::operator==;
using psr::glca::operator<<;
} // namespace psr::glca
