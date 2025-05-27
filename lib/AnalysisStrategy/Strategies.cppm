module;

#include "phasar/AnalysisStrategy.h"

export module phasar.analysisstrategy;

export namespace psr {
using psr::AnalysisSetup;
using psr::AnalysisStrategy;
using psr::DefaultAnalysisSetup;
using psr::DemandDrivenAnalysis;
using psr::HasNoConfigurationType;
using psr::IncrementalUpdateAnalysis;
using psr::ModuleWiseAnalysis;
using psr::toAnalysisStrategy;
using psr::toString;
using psr::VariationalAnalysis;
using psr::operator<<;
} // namespace psr
