#include "IFDSSummaryGenerator.hh"

ostream& operator<<(ostream& os, const SummaryGenerationCTXStrategy& s) {
	switch(s) {
	case SummaryGenerationCTXStrategy::always_all:
		os << "SummaryGenerationCTXStrategy::always_all";
		break;
	case SummaryGenerationCTXStrategy::always_none:
		os << "SummaryGenerationCTXStrategy::always_none";
		break;
	case SummaryGenerationCTXStrategy::all_and_none:
		os << "SummaryGenerationCTXStrategy::all_and_none";
		break;
	case SummaryGenerationCTXStrategy::powerset:
		os << "SummaryGenerationCTXStrategy::powerset";
		break;
	case SummaryGenerationCTXStrategy::all_observed:
		os << "SummaryGenerationCTXStrategy::all_observed";
		break;
	default:
		os << "SummaryGenerationCTXStrategy::error";
		break;
	}
	return os;
}
