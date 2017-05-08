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
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include "../../db/ProjectIRCompiledDB.hh"
#include "DefaultIFDSTabulationProblem.hh"
#include "IFDSTabulationProblem.hh"
#include "icfg/LLVMBasedICFG.hh"
using namespace std;

class IFDSSummaryGenerator {
private:
	vector<string> ToSummarize;
	ProjectIRCompiledDB& IRDB;
	void computeSummary(const string& FunctionName);
	void computeSummary(const llvm::Function* Function);
	set<const llvm::Value*> getInputs(const llvm::Function* Function) {
		set<const llvm::Value*> inputs;
		for (auto& arg : Function->args()) {
				inputs.insert(&arg);
		}
		return inputs;
	}


public:
	IFDSSummaryGenerator(vector<string> FunctionNames);
	virtual ~IFDSSummaryGenerator() = default;
};

#endif /* SRC_ANALYSIS_IFDS_IDE_IFDSSUMMARYGENERATOR_HH_ */
