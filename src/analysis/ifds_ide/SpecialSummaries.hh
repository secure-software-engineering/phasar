/*
 * SpecialSummaries.hh
 *
 *  Created on: 05.05.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_IFDS_IDE_SPECIALSUMMARIES_HH_
#define SRC_ANALYSIS_IFDS_IDE_SPECIALSUMMARIES_HH_

#include <set>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include "../../analysis/ifds_ide/FlowFunction.hh"
#include "../../analysis/ifds_ide/flow_func/Identity.hh"
#include "../../utils/IO.hh"
#include "../../utils/Configuration.hh"
#include "../../utils/ContainerConfiguration.hh"
#include "../../utils/utils.hh"
using namespace std;

template<typename D>
class SpecialSummaries {
private:
	SSMap<string, shared_ptr<FlowFunction<D>>> SpecialSummaryMap;
	vector<string> operators = {"_Znwm", "_Znam", "_ZdlPv", "_ZdaPv"};

	SpecialSummaries() {
		string glibc = readFile(ConfigurationDirectory+GLIBCFunctionListFileName);
		vector<string> glibcfunctions = splitString(glibc, "\n");
		string llvmintrinsics = readFile(ConfigurationDirectory+LLVMIntrinsicFunctionListFileName);
		vector<string> llvmintrinsicfunctions = splitString(llvmintrinsics, "\n");
		SpecialSummaryMap.reserve(glibcfunctions.size()+llvmintrinsicfunctions.size()+operators.size());
		// insert all glibc functions
		for (auto& function_name : glibcfunctions) {
			SpecialSummaryMap.emplace(std::make_pair(function_name, Identity<D>::v()));
		}
		// insert all llvm intrinsic functions
		for (auto& function_name : llvmintrinsicfunctions) {
			SpecialSummaryMap.emplace(std::make_pair(function_name, Identity<D>::v()));
		}
		// insert all C++ special operators
		for (auto& function_name : operators) {
			SpecialSummaryMap.emplace(std::make_pair(function_name, Identity<D>::v()));
		}
	}

public:
	SpecialSummaries(const SpecialSummaries&) = delete;
	SpecialSummaries& operator= (const SpecialSummaries&) = delete;
	SpecialSummaries(SpecialSummaries&&) = delete;
	SpecialSummaries& operator= (SpecialSummaries&&) = delete;
	~SpecialSummaries() = default;

	static SpecialSummaries& getInstance() {
		static SpecialSummaries instance;
		return instance;
	}

	/**
	 * Returns true, when an existing function is overwritten, false otherwise.
	 */
	bool provideCustomSpecialSummary(const string& name, shared_ptr<FlowFunction<D>> flowfunction) {
		SpecialSummaryMap[name] = flowfunction;
		return containsSpecialSummary(name);
	}

	bool containsSpecialSummary(const string& name) {
		return SpecialSummaryMap.find(name) != SpecialSummaryMap.end();
	}

	shared_ptr<FlowFunction<D>> getSpecialSummary(const string& name) {
		return SpecialSummaryMap[name];
	}

	friend ostream& operator<<(ostream& os, const SpecialSummaries<D>& ss) {
		os << "SpecialSummary:\n";
		for (auto& entry : ss.SpecialSummaryMap) {
			os << entry.first << " ";
		}
		return os << "\n";
	}
};

#endif /* SRC_ANALYSIS_IFDS_IDE_SPECIALSUMMARIES_HH_ */
