module;

#include "phasar/Domain.h"

export module phasar.domain;

export namespace psr {
using psr::AnalysisDomain;
using psr::BinaryDomain;
using psr::to_string;
using psr::operator<<;
using psr::Bottom;
using psr::IRDomain;
using psr::JoinLatticeTraits;
using psr::LatticeDomain;
using psr::NonTopBotValue;
using psr::Top;
} // namespace psr

export namespace std {
using std::hash;
} // namespace std
