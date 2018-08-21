/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * ObservedCallingContexts.h
 *
 *  Created on: 14.06.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_OBSERVEDCALLINGCONTEXTS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_OBSERVEDCALLINGCONTEXTS_H_

#include <map>
#include <set>
#include <string>
#include <vector>

namespace psr {

class ObservedCallingContexts {
private:
  // Maps a function to the set of contexts that have been recognized so
  // far
  std::map<std::string, std::set<std::vector<bool>>> ObservedCTX;

public:
  ObservedCallingContexts() = default;
  ~ObservedCallingContexts() = default;
  void addObservedCTX(std::string FName, std::vector<bool> CTX);
  bool containsCTX(std::string FName);
  std::set<std::vector<bool>> getObservedCTX(std::string FName);
  void print();
};
} // namespace psr

#endif
