/*
 * IFDSTypeAnalysis.hh
 *
 *  Created on: 06.12.2016
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TYPE_ANALYSIS_IFDSTYPEANALYSIS_HH_
#define ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TYPE_ANALYSIS_IFDSTYPEANALYSIS_HH_

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <map>
#include <set>
#include <memory>
#include "../../ifds_ide/DefaultIFDSTabulationProblem.hh"
#include "../../ifds_ide/FlowFunction.hh"
#include "../../ifds_ide/icfg/LLVMBasedInterproceduralCFG.hh"
#include "../../ifds_ide/DefaultSeeds.hh"
using namespace std;
class IFDSTypeAnalysis : public DefaultIFDSTabulationProblem
										<const llvm::Instruction*,
										 const llvm::Value*,
										 const llvm::Function*,
										 LLVMBasedInterproceduralICFG&
										> {
public:
	IFDSTypeAnalysis(LLVMBasedInterproceduralICFG& icfg) : DefaultIFDSTabulationProblem(icfg, createZeroValue()) {}

	virtual ~IFDSTypeAnalysis() = default;

	shared_ptr<FlowFunction<const llvm::Value*>> getNormalFlowFunction(const llvm::Instruction* curr,
																	   const llvm::Instruction* succ) override
	{
		cout << "type analysis getNormalFlowFunction()" << endl;
		struct TAFF : FlowFunction<const llvm::Value*> {
			set<const llvm::Value*> computeTargets(const llvm::Value* source) override
			{
				return set<const llvm::Value*>{};
			}
		};
		return make_shared<TAFF>();
	}

	shared_ptr<FlowFunction<const llvm::Value*>> getCallFlowFuntion(const llvm::Instruction* callStmt,
																	const llvm::Function* destMthd) override
	{
		cout << "type analysis getCallFlowFunction()" << endl;
		struct TAFF : FlowFunction<const llvm::Value*> {
			set<const llvm::Value*> computeTargets(const llvm::Value* source) override
			{
				return set<const llvm::Value*>{};
			}
		};
		return make_shared<TAFF>();
	}

	shared_ptr<FlowFunction<const llvm::Value*>> getRetFlowFunction(const llvm::Instruction* callSite,
															  const llvm::Function* calleeMthd,
															  const llvm::Instruction* exitStmt,
															  const llvm::Instruction* retSite) override
	{
		cout << "type analysis getRetFlowFunction()" << endl;
		struct TAFF : FlowFunction<const llvm::Value*> {
			set<const llvm::Value*> computeTargets(const llvm::Value* source) override
			{
				return set<const llvm::Value*>{};
			}
		};
		return make_shared<TAFF>();
	}

	shared_ptr<FlowFunction<const llvm::Value*>> getCallToRetFlowFunction(const llvm::Instruction* callSite,
																	const llvm::Instruction* retSite) override
	{
		cout << "type analysis getCallToRetFlowFunction()" << endl;
		struct TAFF : FlowFunction<const llvm::Value*> {
			set<const llvm::Value*> computeTargets(const llvm::Value* source) override
			{
				return set<const llvm::Value*>{};
			}
		};
		return make_shared<TAFF>();
	}

	map<const llvm::Instruction*, set<const llvm::Value*>> initialSeeds() override
	{
		const llvm::Function* mainfunction = icfg.getModule().getFunction("main");
		const llvm::Instruction* firstinst = &(*(mainfunction->begin()->begin()));
		set<const llvm::Value*> iset{zeroValue()};
		map<const llvm::Instruction*, set<const llvm::Value*>> imap{ {firstinst, iset} };
		return imap;
	}

	const llvm::Value* createZeroValue() override
	{
		// we can just use nullptr as zero value, since we are dealing with pointers all the time
		return nullptr;
	}

};

#endif /* ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TYPE_ANALYSIS_IFDSTYPEANALYSIS_HH_ */
