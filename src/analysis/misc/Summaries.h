#ifndef SUMMARIES_H_
#define SUMMARIES_H_

#include <iostream>
#include <map>
using namespace std;

enum class SummaryGenerationCTXStrategy {
  always_all = 0,
  powerset,
  all_and_none,
  all_observed,
  always_none
};

extern const map<SummaryGenerationCTXStrategy, string>
    SummaryGenerationCTXStrategyToString;

extern const map<string, SummaryGenerationCTXStrategy>
    StringToSummaryGenerationCTXStrategy;

ostream &operator<<(ostream &os, const SummaryGenerationCTXStrategy &s);

#endif