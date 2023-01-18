/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/IFDSIDESolverConfig.h"

#include <ostream>

using namespace std;
using namespace psr;

namespace psr {

IFDSIDESolverConfig::IFDSIDESolverConfig(SolverConfigOptions Options) noexcept
    : Options(Options) {}

bool IFDSIDESolverConfig::followReturnsPastSeeds() const {
  return hasFlag(Options, SolverConfigOptions::FollowReturnsPastSeeds);
}
bool IFDSIDESolverConfig::autoAddZero() const {
  return hasFlag(Options, SolverConfigOptions::AutoAddZero);
}
bool IFDSIDESolverConfig::computeValues() const {
  return hasFlag(Options, SolverConfigOptions::ComputeValues);
}
bool IFDSIDESolverConfig::recordEdges() const {
  return hasFlag(Options, SolverConfigOptions::RecordEdges);
}
bool IFDSIDESolverConfig::emitESG() const {
  return hasFlag(Options, SolverConfigOptions::EmitESG);
}
bool IFDSIDESolverConfig::computePersistedSummaries() const {
  return hasFlag(Options, SolverConfigOptions::ComputePersistedSummaries);
}

void IFDSIDESolverConfig::setFollowReturnsPastSeeds(bool Set) {
  setFlag(Options, SolverConfigOptions::FollowReturnsPastSeeds, Set);
}
void IFDSIDESolverConfig::setAutoAddZero(bool Set) {
  setFlag(Options, SolverConfigOptions::AutoAddZero, Set);
}
void IFDSIDESolverConfig::setComputeValues(bool Set) {
  setFlag(Options, SolverConfigOptions::ComputeValues, Set);
}
void IFDSIDESolverConfig::setRecordEdges(bool Set) {
  setFlag(Options, SolverConfigOptions::RecordEdges, Set);
}
void IFDSIDESolverConfig::setEmitESG(bool Set) {
  setFlag(Options, SolverConfigOptions::EmitESG, Set);
}
void IFDSIDESolverConfig::setComputePersistedSummaries(bool Set) {
  setFlag(Options, SolverConfigOptions::ComputePersistedSummaries, Set);
}

void IFDSIDESolverConfig::setConfig(SolverConfigOptions Opt) { Options = Opt; }

ostream &operator<<(ostream &OS, const IFDSIDESolverConfig &SC) {
  return OS << "IFDSIDESolverConfig:\n"
            << "\tfollowReturnsPastSeeds: " << SC.followReturnsPastSeeds()
            << "\n"
            << "\tautoAddZero: " << std::boolalpha << SC.autoAddZero() << "\n"
            << "\tcomputeValues: " << SC.computeValues() << "\n"
            << "\trecordEdges: " << SC.recordEdges() << "\n"
            << "\tcomputePersistedSummaries: " << SC.computePersistedSummaries()
            << "\n"
            << "\temitESG: " << SC.emitESG();
}

} // namespace psr
