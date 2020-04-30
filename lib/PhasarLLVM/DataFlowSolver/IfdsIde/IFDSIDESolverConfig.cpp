/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <ostream>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSIDESolverConfig.h"

using namespace std;
using namespace psr;

namespace psr {
IFDSIDESolverConfig::IFDSIDESolverConfig() {
  setFlag(
      options, SolverConfigOptions::EmitESG,
      PhasarConfig::getPhasarConfig().VariablesMap().count("emit-esg-as-dot"));
}
IFDSIDESolverConfig::IFDSIDESolverConfig(SolverConfigOptions options)
    : options(options) {}

bool IFDSIDESolverConfig::followReturnsPastSeeds() const {
  return hasFlag(options, SolverConfigOptions::FollowReturnsPastSeeds);
}
bool IFDSIDESolverConfig::autoAddZero() const {
  return hasFlag(options, SolverConfigOptions::AutoAddZero);
}
bool IFDSIDESolverConfig::computeValues() const {
  return hasFlag(options, SolverConfigOptions::ComputeValues);
}
bool IFDSIDESolverConfig::recordEdges() const {
  return hasFlag(options, SolverConfigOptions::RecordEdges);
}
bool IFDSIDESolverConfig::emitESG() const {
  return hasFlag(options, SolverConfigOptions::EmitESG);
}
bool IFDSIDESolverConfig::computePersistedSummaries() const {
  return hasFlag(options, SolverConfigOptions::ComputePersistedSummaries);
}

void IFDSIDESolverConfig::setFollowReturnsPastSeeds(bool set) {
  setFlag(options, SolverConfigOptions::FollowReturnsPastSeeds, set);
}
void IFDSIDESolverConfig::setAutoAddZero(bool set) {
  setFlag(options, SolverConfigOptions::AutoAddZero, set);
}
void IFDSIDESolverConfig::setComputeValues(bool set) {
  setFlag(options, SolverConfigOptions::ComputeValues, set);
}
void IFDSIDESolverConfig::setRecordEdges(bool set) {
  setFlag(options, SolverConfigOptions::RecordEdges, set);
}
void IFDSIDESolverConfig::setEmitESG(bool set) {
  setFlag(options, SolverConfigOptions::EmitESG, set);
}
void IFDSIDESolverConfig::setComputePersistedSummaries(bool set) {
  setFlag(options, SolverConfigOptions::ComputePersistedSummaries, set);
}

ostream &operator<<(ostream &OS, const IFDSIDESolverConfig &SC) {
  return OS << "IFDSIDESolverConfig:\n"
            << "\tfollowReturnsPastSeeds: " << SC.followReturnsPastSeeds()
            << "\n"
            << "\tautoAddZero: " << SC.autoAddZero() << "\n"
            << "\tcomputeValues: " << SC.computeValues() << "\n"
            << "\trecordEdges: " << SC.recordEdges() << "\n"
            << "\tcomputePersistedSummaries: " << SC.computePersistedSummaries()
            << "\n"
            << "\temitESG: " << SC.emitESG();
}

} // namespace psr
