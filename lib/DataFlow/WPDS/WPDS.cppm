module;

#include "phasar/DataFlow/WPDS/PAutomaton.h"
#include "phasar/DataFlow/WPDS/Semiring.h"
#include "phasar/DataFlow/WPDS/WPDS.h"
#include "phasar/DataFlow/WPDS/WPDSProblem.h"
#include "phasar/DataFlow/WPDS/WPDSRule.h"
#include "phasar/DataFlow/WPDS/WPDSSolver.h"
#include "phasar/DataFlow/WPDS/WPDSSolverResults.h"

export module phasar.dataflow.wpds;

export namespace psr::wpds {
using psr::wpds::BoundedIdempotentSemiring;
using psr::wpds::kEpsilonSym;
using psr::wpds::PAutomaton;
using psr::wpds::WeightedPushdownSystem;
using psr::wpds::WPDSAnalysisDomain;
using psr::wpds::WPDSProblem;
using psr::wpds::WPDSRule;
using psr::wpds::WPDSRuleKind;
using psr::wpds::WPDSSolver;
using psr::wpds::WPDSSolverResults;
} // namespace psr::wpds
