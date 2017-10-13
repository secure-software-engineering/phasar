#include "Summaries.hh"

const map<SummaryGenerationCTXStrategy, string>
    SummaryGenerationCTXStrategyToString = {
        {SummaryGenerationCTXStrategy::always_all, "always_all"},
        {SummaryGenerationCTXStrategy::always_none, "always_none"},
        {SummaryGenerationCTXStrategy::all_and_none, "all_and_none"},
        {SummaryGenerationCTXStrategy::powerset, "powerset"},
        {SummaryGenerationCTXStrategy::all_observed, "all_observed"}};

const map<string, SummaryGenerationCTXStrategy>
    StringToSummaryGenerationCTXStrategy = {
        {"always_all", SummaryGenerationCTXStrategy::always_all},
        {"always_none", SummaryGenerationCTXStrategy::always_none},
        {"all_and_none", SummaryGenerationCTXStrategy::all_and_none},
        {"powerset", SummaryGenerationCTXStrategy::powerset},
        {"all_observed", SummaryGenerationCTXStrategy::all_observed}

};

ostream& operator<<(ostream& os, const SummaryGenerationCTXStrategy& s) {
  return os << SummaryGenerationCTXStrategyToString.at(s);
}