/*
 * IFDSSummaryGenerator.hh
 *
 *  Created on: 03.05.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARYGENERATOR_HH_
#define SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARYGENERATOR_HH_

#include <string>
#include <vector>
#include <memory>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include "../../db/ProjectIRCompiledDB.hh"
#include "DefaultIFDSTabulationProblem.hh"
#include "IFDSTabulationProblem.hh"
#include "solver/IFDSSolver.hh"
#include "../ifds_ide/FlowFunction.hh"
#include "../ifds_ide/flow_func/GenAll.hh"
#include "icfg/LLVMBasedICFG.hh"
#include "icfg/ICFG.hh"
#include "../../utils/utils.hh"
#include "../../lib/LLVMShorthands.hh"
#include "solver/SummaryGenerator.hh"
using namespace std;

template<typename D, typename I, typename ConcreteIFDSTabulationProblem>
class IFDSSummaryGenerator {
private:
	const llvm::Function* toSummarize;
	const I& icfg;
	llvm::LLVMContext& c;

	vector<D> getInputs() {
		vector<D> inputs;
		// collect arguments
		for (auto& arg : toSummarize->args()) {
				inputs.push_back(&arg);
		}
		// collect global values
		auto globals = globalValuesUsedinFunction(toSummarize);
		inputs.insert(inputs.end(), globals.begin(), globals.end());
		return inputs;
	}

	vector<bool> generateBitPattern(const vector<D>& inputs,
																 	const set<D>& subset) {
		// initialize all bits to zero
		vector<bool> bitpattern(inputs.size(), 0);
		if (subset.empty()) {
			return bitpattern;
		}
		for (auto elem : subset) {
			for (size_t i = 0; i < inputs.size(); ++i) {
				if (elem == inputs[i]) {
					bitpattern[i] = 1;
				}
			}
		}
		return bitpattern;
	}

	class CTXFunctionProblem : public ConcreteIFDSTabulationProblem {
	public:
		const llvm::Instruction* start;
		set<D> facts;

		CTXFunctionProblem(const llvm::Instruction* start,
											 set<D> facts,
											 I icfg,
											 llvm::LLVMContext& c)
		: ConcreteIFDSTabulationProblem(icfg, c), start(start), facts(facts) {
			this->solver_config.followReturnsPastSeeds = false;
			this->solver_config.autoAddZero = true;
			this->solver_config.computeValues = true;
			this->solver_config.recordEdges = false;
			this->solver_config.computePersistedSummaries = false;
		}

		virtual map<const llvm::Instruction*, set<D>> initialSeeds() override {
			map<const llvm::Instruction*, set<D>> seeds;
			seeds.insert(make_pair(start, facts));
			return seeds;
		}
	};



public:
	IFDSSummaryGenerator(const llvm::Function* Function, I icfg, llvm::LLVMContext& c)
												: toSummarize(Function), icfg(icfg), c(c) {}

	virtual ~IFDSSummaryGenerator() = default;

	set<pair<vector<bool>, shared_ptr<FlowFunction<D>>>> generateSummaryFlowFunction() {
		set<pair<vector<bool>, shared_ptr<FlowFunction<D>>>> summary;
		// compute combinations and start solver on all CTXFunctionProblems
		vector<D> inputs = getInputs();
		set<D> inputset;
		inputset.insert(inputs.begin(), inputs.end());
		set<set<D>> powerset = computePowerSet(inputset);
		for (auto subset : powerset) {
			cout << "Generate summary for specific context\n";
			CTXFunctionProblem functionProblem(&(*toSummarize->begin()->begin()), subset, icfg, c);
			LLVMIFDSSolver<D, LLVMBasedICFG&> solver(functionProblem, false);
			solver.solve();
			// get the result at the end of this function and
			set<D> results;
			// create a flow function from this set using the GenAll class
			summary.insert(make_pair(generateBitPattern(inputs, subset),
							  	  					 make_shared<GenAll<D>>(results, functionProblem.zeroValue())));
		}
		return summary;
	}
};

#endif /* SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARYGENERATOR_HH_ */
