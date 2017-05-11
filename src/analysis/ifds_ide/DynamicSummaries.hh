/*
 * DynamicSummaries.hh
 *
 *  Created on: 08.05.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_IFDS_IDE_DYNAMICSUMMARIES_HH_
#define SRC_ANALYSIS_IFDS_IDE_DYNAMICSUMMARIES_HH_

#include <algorithm>
#include <vector>
#include <string>
#include <memory>
#include "../../analysis/ifds_ide/FlowFunction.hh"
#include "../../analysis/ifds_ide/flow_func/GenAll.hh"
#include "../../utils/ContainerConfiguration.hh"
using namespace std;

template<typename D>
class DynamicSummaries {
private:
	/**
	 * Stores the summary for a function specified by a name which holds under a
	 * specific context described by a bit-pattern represented as vector<bool> type.
	 */
	DSMap<string, DSMap<vector<bool>, shared_ptr<GenAll<D>>>> SummaryMap;

public:
	DynamicSummaries() = default;
	~DynamicSummaries() = default;

	bool containsSummary(const string& name) {
		return SummaryMap.find(name) != SummaryMap.end();
	}

	void insertSummary(const string& name, const vector<bool>& context, set<D> result) {
		SummaryMap[name].insert({ context, make_shared<GenAll>(result) });
	}

	set<D> getSummary(const string& name, const vector<bool>& context) {
		return SummaryMap[name][context];
	}

	void print() {
		cout << "DynamicSummaries:\n";
		for (auto& entry : SummaryMap) {
			cout << "Function: " << entry.first << "\n";
			for (auto& context_summaries : entry.second) {
				cout << "Context: ";
				for_each(context_summaries.first.begin(), context_summaries.first.end(), [](bool b) {
					cout << b;
				});
				cout << "\n";
				cout << "Beg results:\n";
				for (auto& result : context_summaries.second) {
					//result->dump();
					cout << "fixme\n";
				}
				cout << "End results!\n";
			}
		}
	}
};

#endif
