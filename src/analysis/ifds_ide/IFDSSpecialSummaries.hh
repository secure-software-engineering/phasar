/*
 * SpecialSummaries.hh
 *
 *  Created on: 05.05.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_IFDS_IDE_IFDSSPECIALSUMMARIES_HH_
#define SRC_ANALYSIS_IFDS_IDE_IFDSSPECIALSUMMARIES_HH_

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
class IFDSSpecialSummaries {
private:
	SSMap<string, shared_ptr<FlowFunction<D>>> SpecialSummaryMap;
	vector<string> operators = {"_Znwm", "_Znam", "_ZdlPv", "_ZdaPv"};

	// Constructs the SpecialSummaryMap such that it contains all glibc, 
	// llvm.intrinsics and C++'s new, new[], delete, delete[] with identity
	// flow functions.
	IFDSSpecialSummaries() {
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
	IFDSSpecialSummaries(const IFDSSpecialSummaries&) = delete;
	IFDSSpecialSummaries& operator= (const IFDSSpecialSummaries&) = delete;
	IFDSSpecialSummaries(IFDSSpecialSummaries&&) = delete;
	IFDSSpecialSummaries& operator= (IFDSSpecialSummaries&&) = delete;
	~IFDSSpecialSummaries() = default;

	static IFDSSpecialSummaries& getInstance() {
		static IFDSSpecialSummaries instance;
		return instance;
	}

	/**
	 * Returns true, when an existing function is overwritten, false otherwise.
	 */
	bool provideCustomSpecialSummary(const string& name, shared_ptr<FlowFunction<D>> flowfunction) {
		bool Override = containsSpecialSummary(name);
		SpecialSummaryMap[name] = flowfunction;
		return Override;
	}

	bool containsSpecialSummary(const llvm::Function* function) {
		return containsSpecialSummary(function->getName().str());
	}

	bool containsSpecialSummary(const string& name) {
		return SpecialSummaryMap.count(name);
	}

	shared_ptr<FlowFunction<D>> getSpecialSummary(const llvm::Function* function) {
		return getSpecialSummary(function->getName().str());
	}

	shared_ptr<FlowFunction<D>> getSpecialSummary(const string& name) {
		return SpecialSummaryMap[name];
	}

	friend ostream& operator<<(ostream& os, const IFDSSpecialSummaries<D>& ss) {
		os << "SpecialSummary:\n";
		for (auto& entry : ss.SpecialSummaryMap) {
			os << entry.first << " ";
		}
		return os << "\n";
	}
};

#endif /* SRC_ANALYSIS_IFDS_IDE_IFDSSPECIALSUMMARIES_HH_ */
