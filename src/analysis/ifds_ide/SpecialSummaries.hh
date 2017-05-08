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
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include "../../analysis/ifds_ide/FlowFunction.hh"
#include "../../analysis/ifds_ide/flow_func/Identity.hh"
#include "../../utils/IO.hh"
#include "../../utils/Configuration.hh"
#include "../../utils/ContainerConfiguration.hh"
using namespace std;

template<typename D>
class SpecialSummaries {
private:
	SSMap<string, shared_ptr<FlowFunction<D>>> GLIBCSummaryMap;
	SSMap<string, shared_ptr<FlowFunction<D>>> LLVMIntrinsicsSummaryMap;
	SSMap<string, shared_ptr<FlowFunction<D>>> LanguageSummaryMap;

	SpecialSummaries() : GLIBCSummaryMap(getGLIBCSummaryMap()),
			 	 	 	 	 	 	 	 	 LLVMIntrinsicsSummaryMap(getLLVMIntrinsicsSummaryMap()),
											 LanguageSummaryMap(getLanguageSummaryMap()) {}

	SSMap<string, shared_ptr<FlowFunction<D>>> getGLIBCSummaryMap() {
		string contents = readFile(ConfigurationDirectory+GLIBCFunctionListFileName);
		vector<string> glibcfunctions;
		boost::split(glibcfunctions, contents, boost::is_any_of("\n"), boost::token_compress_on);
		SSMap<string, shared_ptr<FlowFunction<D>>> summary_map;
		summary_map.reserve(glibcfunctions.size());
		for (auto& function_name : glibcfunctions) {
			summary_map.insert({ function_name, Identity<D>::v() });
		}
		return summary_map;
	}

	SSMap<string, shared_ptr<FlowFunction<D>>> getLLVMIntrinsicsSummaryMap() {
		string contents = readFile(ConfigurationDirectory+LLVMIntrinsicFunctionListFileName);
		vector<string> llvmintrinsicfunctions;
		boost::split(llvmintrinsicfunctions, contents, boost::is_any_of("\n"), boost::token_compress_on);
		SSMap<string, shared_ptr<FlowFunction<D>>> summary_map;
		summary_map.reserve(llvmintrinsicfunctions.size());
		for (auto& function_name : llvmintrinsicfunctions) {
				summary_map.insert({ function_name, Identity<D>::v() });
		}
		return summary_map;
	}

	SSMap<string, shared_ptr<FlowFunction<D>>> getLanguageSummaryMap() {
		// contains operators: 'new', 'new[]', 'delete', 'delete[]'
		static vector<string> operators = {"_Znwm", "_Znam", "_ZdlPv", "_ZdaPv"};
		SSMap<string, shared_ptr<FlowFunction<D>>> summary_map;
		summary_map.reserve(operators.size());
		for (auto& operator_name : operators) {
			summary_map.insert({ operator_name, Identity<D>::v() });
		}
		return summary_map;
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

	void provideCustomGLIBCSummary(const string& name, shared_ptr<FlowFunction<D>> flowfunction) {
		GLIBCSummaryMap[name] = flowfunction;
	}

	void provideCustomLLVMInstrinsicSummary(const string& name, shared_ptr<FlowFunction<D>> flowfunction) {
		LLVMIntrinsicsSummaryMap[name] = flowfunction;
	}

	void provideCustomLanguageSummary(const string& name, shared_ptr<FlowFunction<D>> flowfunction) {
		LanguageSummaryMap[name] = flowfunction;
	}

	bool containsGLIBCSummary(const string& name) {
		return GLIBCSummaryMap.find(name) != GLIBCSummaryMap.end();
	}

	bool containsLLVMIntrinsicSummary(const string& name) {
		return LLVMIntrinsicsSummaryMap.find(name) != LLVMIntrinsicsSummaryMap.end();
	}

	bool containsLanguageSummary(const string& name) {
		return LanguageSummaryMap.find(name) != LanguageSummaryMap.end();
	}

	shared_ptr<FlowFunction<D>> getGLIBCSummary(const string& name) {
		return GLIBCSummaryMap[name];
	}

	shared_ptr<FlowFunction<D>> getLLVMInstrinsicSummary(const string& name) {
		return LLVMIntrinsicsSummaryMap[name];
	}

	shared_ptr<FlowFunction<D>> getLanguageSummary(const string& name) {
		return LanguageSummaryMap[name];
	}

//	friend ostream& operator<<(ostream& os, const SpecialSummaries& ss) {
//		return os;
//	}
};

#endif /* SRC_ANALYSIS_IFDS_IDE_SPECIALSUMMARIES_HH_ */
