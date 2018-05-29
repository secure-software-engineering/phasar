/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef SUMMARYSTRATEGY_H_
#define SUMMARYSTRATEGY_H_

#include <iostream>
#include <map>
using namespace std;

namespace psr{

enum class SummaryGenerationStrategy {
  always_all = 0,
  powerset,
  all_and_none,
  all_observed,
  always_none
};

extern const map<SummaryGenerationStrategy, string>
    SummaryGenerationStrategyToString;

extern const map<string, SummaryGenerationStrategy>
    StringToSummaryGenerationStrategy;

ostream &operator<<(ostream &os, const SummaryGenerationStrategy &s);

}//nmaespace psr

#endif