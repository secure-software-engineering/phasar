/*
 * IFDSSummaryGenerator.hh
 *
 *  Created on: 03.05.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_IFDS_IDE_LLVMIFDSSUMMARYGENERATOR_HH_
#define SRC_ANALYSIS_IFDS_IDE_LLVMIFDSSUMMARYGENERATOR_HH_

#include "../../db/ProjectIRCompiledDB.hh"
#include "../../lib/LLVMShorthands.hh"
#include "../../utils/utils.hh"
#include "../ifds_ide/FlowFunction.hh"
#include "../ifds_ide/flow_func/GenAll.hh"
#include "DefaultIFDSTabulationProblem.hh"
#include "IFDSTabulationProblem.hh"
#include "icfg/ICFG.hh"
#include "icfg/LLVMBasedICFG.hh"
#include "solver/LLVMIFDSSolver.hh"
#include "solver/IFDSSummaryGenerator.hh"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <string>
#include <vector>
using namespace std;

template <typename I, typename ConcreteIFDSTabulationProblem>
class LLVMIFDSSummaryGenerator
    : public IFDSSummaryGenerator<const llvm::Instruction *,
                                  const llvm::Value *, const llvm::Function *,
                                  I, ConcreteIFDSTabulationProblem,
                                  LLVMIFDSSolver<const llvm::Value*, I>> {
private:
  virtual vector<const llvm::Value *> getInputs() {
    vector<const llvm::Value *> inputs;
    // collect arguments
    for (auto &arg : this->toSummarize->args()) {
      inputs.push_back(&arg);
    }
    // collect global values
    auto globals = globalValuesUsedinFunction(this->toSummarize);
    inputs.insert(inputs.end(), globals.begin(), globals.end());
    return inputs;
  }

  virtual vector<bool> generateBitPattern(const vector<const llvm::Value *> &inputs,
                                          const set<const llvm::Value *> &subset) {
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

public:
  LLVMIFDSSummaryGenerator(const llvm::Function *F, I icfg,
                           llvm::LLVMContext &c)
      : IFDSSummaryGenerator<const llvm::Instruction *,
        										 const llvm::Value *,
														 const llvm::Function *,
														 I,
														 ConcreteIFDSTabulationProblem,
														 LLVMIFDSSolver<const llvm::Value*, I>>(F, icfg, c) {}

  virtual ~LLVMIFDSSummaryGenerator() = default;
  // private:
  //	const llvm::Function* toSummarize;
  //	const I& icfg;
  //	llvm::LLVMContext& c;
  //
  //	vector<D> getInputs() {
  //		vector<D> inputs;
  //		// collect arguments
  //		for (auto& arg : toSummarize->args()) {
  //				inputs.push_back(&arg);
  //		}
  //		// collect global values
  //		auto globals = globalValuesUsedinFunction(toSummarize);
  //		inputs.insert(inputs.end(), globals.begin(), globals.end());
  //		return inputs;
  //	}
  //
  //	vector<bool> generateBitPattern(const vector<D>& inputs,
  //																 	const
  //set<D>& subset) {
  //		// initialize all bits to zero
  //		vector<bool> bitpattern(inputs.size(), 0);
  //		if (subset.empty()) {
  //			return bitpattern;
  //		}
  //		for (auto elem : subset) {
  //			for (size_t i = 0; i < inputs.size(); ++i) {
  //				if (elem == inputs[i]) {
  //					bitpattern[i] = 1;
  //				}
  //			}
  //		}
  //		return bitpattern;
  //	}
  //
  //	class CTXFunctionProblem : public ConcreteIFDSTabulationProblem {
  //	public:
  //		const llvm::Instruction* start;
  //		set<D> facts;
  //
  //		CTXFunctionProblem(const llvm::Instruction* start,
  //											 set<D>
  //facts,
  //											 I
  //icfg,
  //											 llvm::LLVMContext&
  //c)
  //		: ConcreteIFDSTabulationProblem(icfg, c), start(start), facts(facts)
  //{
  //			this->solver_config.followReturnsPastSeeds = false;
  //			this->solver_config.autoAddZero = true;
  //			this->solver_config.computeValues = true;
  //			this->solver_config.recordEdges = false;
  //			this->solver_config.computePersistedSummaries = false;
  //		}
  //
  //		virtual map<const llvm::Instruction*, set<D>> initialSeeds()
  //override {
  //			map<const llvm::Instruction*, set<D>> seeds;
  //			seeds.insert(make_pair(start, facts));
  //			return seeds;
  //		}
  //	};
  //
  //
  //
  // public:
  //	LLVMIFDSSummaryGenerator(const llvm::Function* Function, I icfg,
  //llvm::LLVMContext& c)
  //												: toSummarize(Function),
  //icfg(icfg), c(c) {}
  //
  //	virtual ~LLVMIFDSSummaryGenerator() = default;
  //
  //	set<pair<vector<bool>, shared_ptr<FlowFunction<D>>>>
  //generateSummaryFlowFunction() {
  //		set<pair<vector<bool>, shared_ptr<FlowFunction<D>>>> summary;
  //		// compute combinations and start solver on all
  //CTXFunctionProblems
  //		vector<D> inputs = getInputs();
  //		set<D> inputset;
  //		inputset.insert(inputs.begin(), inputs.end());
  //		set<set<D>> powerset = computePowerSet(inputset);
  //		for (auto subset : powerset) {
  //			cout << "Generate summary for specific context\n";
  //			CTXFunctionProblem
  //functionProblem(&(*toSummarize->begin()->begin()), subset, icfg, c);
  //			LLVMIFDSSolver<D, LLVMBasedICFG&> solver(functionProblem,
  //false);
  //			solver.solve();
  //			// get the result at the end of this function and
  //			set<D> results;
  //			// create a flow function from this set using the GenAll
  //class
  //			summary.insert(make_pair(generateBitPattern(inputs,
  //subset),
  //							  	  					 make_shared<GenAll<D>>(results,
  //functionProblem.zeroValue())));
  //		}
  //		return summary;
  //	}
};

#endif /* SRC_ANALYSIS_IFDS_IDE_LLVMIFDSSUMMARYGENERATOR_HH_ */
