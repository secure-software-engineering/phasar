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
using namespace std;

template<typename D, typename N, typename I, typename IFDSTabulationProblem>
class IFDSSummaryGenerator {
private:
	const llvm::Function* toSummarize;
	const I& icfg;
	llvm::LLVMContext& c;

	vector<const llvm::Value*> getInputs() {
		vector<const llvm::Value*> inputs;
		// collect arguments
		for (auto& arg : toSummarize->args()) {
				inputs.push_back(&arg);
		}
		// collect global values
		for (auto& BB : *toSummarize) {
			for (auto& Inst : BB) {
				for (auto& Op : Inst.operands()) {
					if (llvm::GlobalValue* G = llvm::dyn_cast<llvm::GlobalValue>(Op)) {
						inputs.push_back(G);
					}
				}
			}
		}
		return inputs;
	}

	vector<bool> generateBitPattern(const vector<llvm::Value*>& inputs,
																 	const set<llvm::Value*>& subset) {
		vector<bool> bitpattern;
		bitpattern.reserve(inputs.size());
		if (subset.empty()) {
			return bitpattern;
		}
		for (auto elem : subset) {
			for (int i = 0; i < inputs.size(); ++i) {
				if (elem == inputs[i]) {
					bitpattern[i] = 1;
				}
			}
		}
		return bitpattern;
	}

	class CTXFunctionProblem : public IFDSTabulationProblem {
	public:
		const llvm::Instruction* start;
		set<const llvm::Value*> facts;

		CTXFunctionProblem(N start,
											 set<D> facts,
											 I icfg,
											 llvm::LLVMContext& c)
		: IFDSTabulationProblem(icfg, c), start(start), facts(facts) {}

		virtual map<N, set<D>> initialSeeds() override {
			map<N, set<D>> seeds;
			seeds.insert(make_pair(start, facts));
			return seeds;
		}
	};



public:
	IFDSSummaryGenerator(const llvm::Function* Function, I icfg, llvm::LLVMContext& c)
												: toSummarize(Function), icfg(icfg), c(c) {}

	virtual ~IFDSSummaryGenerator() = default;

	set<pair<vector<bool>, shared_ptr<FlowFunction<D>>>> generateIFDSSummary() {
		set<pair<vector<bool>, shared_ptr<FlowFunction<D>>>> summary;
		// compute combinations and start solver on all CTXFunctionProblems
		vector<const llvm::Value*> inputs = getInputs();
		set<const llvm::Value*> inputset;
		inputset.insert(inputs.begin(), inputs.end());
		set<set<const llvm::Value*>> powerset = computePowerSet(inputset);
		for (auto subset : powerset) {
			cout << "Generate summary for specific context\n";
			CTXFunctionProblem functionProblem(&(*toSummarize->begin()->begin()), subset, icfg, c);
			LLVMIFDSSolver<const llvm::Value*, LLVMBasedICFG&> solver(functionProblem, false);
			solver.solve();
			// generate bitpattern and insert into summaryset
			//summarySet.insert(make_pair(generateBitPattern(inputs, subset),
			//														make_shared<GenAll<const llvm::Value*>>({}, 0)));
		}
		return summary;
	}
};

#endif /* SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARYGENERATOR_HH_ */
