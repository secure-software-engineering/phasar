/*
 * ICFG.h
 *
 *  Created on: 17.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_ICFG_H_
#define ANALYSIS_IFDS_IDE_ICFG_H_

#include "../../config/ContainerConfiguration.h"
#include "CFG.h"
#include <iostream>
#include <set>
#include <string>
#include <vector>
using namespace std;

enum class CallGraphAnalysisType { CHA, RTA, DTA, VTA, OTF };

extern const map<string, CallGraphAnalysisType> StringToCallGraphAnalysisType;

extern const map<CallGraphAnalysisType, string> CallGraphAnalysisTypeToString;

ostream &operator<<(ostream &os, const CallGraphAnalysisType &CGA);

template <typename N, typename M> class ICFG : public CFG<N, M> {
public:
  virtual ~ICFG() = default;

  virtual bool isCallStmt(N stmt) = 0;

  virtual M getMethod(const string &fun) = 0;

  virtual set<N> allNonCallStartNodes() = 0;

  virtual set<M> getCalleesOfCallAt(N stmt) = 0;

  virtual set<N> getCallersOf(M fun) = 0;

  virtual set<N> getCallsFromWithin(M fun) = 0;

  virtual set<N> getStartPointsOf(M fun) = 0;

  virtual set<N> getExitPointsOf(M fun) = 0;

  virtual set<N> getReturnSitesOfCallAt(N stmt) = 0;
};

#endif /* ANALYSIS_ICFG_HH_ */
