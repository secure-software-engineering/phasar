/*
 * ICFG.cpp
 *
 *  Created on: 17.08.2016
 *      Author: pdschbrt
 */

#include "ICFG.hh"

const map<string, CallGraphAnalysisType> CallGraphAnalysisTypeMap = { { "CHA", CallGraphAnalysisType::CHA },
																																			{ "RTA", CallGraphAnalysisType::RTA },
																																			{ "DTA", CallGraphAnalysisType::DTA },
																																			{ "VTA", CallGraphAnalysisType::VTA },
																																			{ "OTF", CallGraphAnalysisType::OTF } };

ostream &operator<<(ostream &os, const CallGraphAnalysisType &CGA) {
	switch (CGA) {
		case CallGraphAnalysisType::CHA:
			return os << "CallGraphAnalysisType::CHA";
			break;
		case CallGraphAnalysisType::RTA:
			return os << "CallGraphAnalysisType::RTA";
			break;
		case CallGraphAnalysisType::DTA:
			return os << "CallGraphAnalysisType::DTA";
			break;
		case CallGraphAnalysisType::VTA:
			return os << "CallGraphAnalysisType::VTA";
			break;
		case CallGraphAnalysisType::OTF:
			return os << "CallGraphAnalysisType::OTF";
			break;
		default:
			return os << "CallGraphAnalysisType::error";
			break;
	}
}
