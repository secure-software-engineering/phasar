/*
 * ICFG.hh
 *
 *  Created on: 17.08.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_ICFG_HH_
#define ANALYSIS_IFDS_IDE_ICFG_HH_

#include "../../utils/ContainerConfiguration.hh"
#include "CFG.hh"
#include <iostream>
#include <set>
#include <string>
#include <vector>
using namespace std;

enum class CallType { none = 0, call = 1, unavailable = 2 };

enum class CallGraphAnalysisType {
  CHA,
  RTA,
  DTA,
  VTA,
  OTF
};

extern const map<string, CallGraphAnalysisType> CallGraphAnalysisTypeMap;

ostream &operator<<(ostream &os, const CallType &CT);

ostream &operator<<(ostream &os, const CallGraphAnalysisType &CGA);

template <typename N, typename M> class ICFG : public CFG<N, M> {
public:
  virtual ~ICFG() = default;

  /**
   * We return an int rather than a boolean value, since we would also like to
   * distinguish between different categories of functions that are called.
   * A class that inherits from the ICFG interface can define a suitable
   * enumeration that represents various kinds of categories. Different
   * categories can than be treated different within the solver (e.g. special
   * summaries may be used, etc.). IMPORTANT: by convention returning 0
   * indicates a non-call statement, returning 1 indicates a usual function call
   * that should be treated as a usual function by the solver.
   * 2 represents special functions like ones defined in glibc or llvm intrinsic
   * functions. At last 3 represents that the function being called is not
   * available.
   */
  virtual CallType isCallStmt(N stmt) = 0;

  virtual M getMethod(const string& fun) = 0;

  virtual set<N> allNonCallStartNodes() = 0;

  virtual set<M> getCalleesOfCallAt(N stmt) = 0;

  virtual set<N> getCallersOf(M fun) = 0;

  virtual set<N> getCallsFromWithin(M fun) = 0;

  virtual set<N> getStartPointsOf(M fun) = 0;

  virtual set<N> getExitPointsOf(M fun) = 0;

  virtual set<N> getReturnSitesOfCallAt(N stmt) = 0;
};

#endif /* ANALYSIS_ICFG_HH_ */
