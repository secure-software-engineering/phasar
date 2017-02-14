/*
 * ide_taint_analysis.hh
 *
 *  Created on: 10.01.2017
 *      Author: pdschbrt
 */

#ifndef ANALYSIS_IFDS_IDE_PROBLEMS_IDE_TAINT_ANALYSIS_IDETAINTANALYSIS_HH_
#define ANALYSIS_IFDS_IDE_PROBLEMS_IDE_TAINT_ANALYSIS_IDETAINTANALYSIS_HH_

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/LLVMContext.h>
#include <map>
#include <set>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "../../ifds_ide/DefaultSeeds.hh"
#include "../../ifds_ide/FlowFunction.hh"
#include "../../ifds_ide/flow_func/Identity.hh"
#include "../../ifds_ide/DefaultIDETabulationProblem.hh"
#include "../../ifds_ide/edge_func/EdgeIdentity.hh"
#include "../../ifds_ide/icfg/LLVMBasedInterproceduralCFG.hh"
using namespace std;

class IDETaintAnalysis : public DefaultIDETabulationProblem<const llvm::Instruction*,
										 	 	 	 	 	const llvm::Value*,
															const llvm::Function*,
															const llvm::Value*,
															LLVMBasedInterproceduralICFG&
															> {
private:
	llvm::LLVMContext& context;

public:
	set<string> source_functions = { "fread", "read" };
	// keep in mind that 'char** argv' of main is a source for tainted values as well
	set<string> sink_functions = { "fwrite", "write", "printf" };
	bool set_contains_str(set<string> s, string str) { return s.find(str) != s.end(); }

	IDETaintAnalysis(LLVMBasedInterproceduralICFG& icfg, llvm::LLVMContext& c) : DefaultIDETabulationProblem(icfg), context(c)
	{
		DefaultIDETabulationProblem::zerovalue = createZeroValue();
	}

	~IDETaintAnalysis() = default;

	// start formulating our analysis by specifying the parts required for IFDS

	shared_ptr<FlowFunction<const llvm::Value*>> getNormalFlowFunction(const llvm::Instruction* curr,
																	   const llvm::Instruction* succ) override
	{
		return Identity<const llvm::Value*>::v();
	}

	shared_ptr<FlowFunction<const llvm::Value*>> getCallFlowFuntion(const llvm::Instruction* callStmt,
																	const llvm::Function* destMthd) override
	{
		return Identity<const llvm::Value*>::v();
	}

	shared_ptr<FlowFunction<const llvm::Value*>> getRetFlowFunction(const llvm::Instruction* callSite,
															  	  	const llvm::Function* calleeMthd,
																	const llvm::Instruction* exitStmt,
																	const llvm::Instruction* retSite) override
	{
		return Identity<const llvm::Value*>::v();
	}

	shared_ptr<FlowFunction<const llvm::Value*>> getCallToRetFlowFunction(const llvm::Instruction* callSite,
																		  const llvm::Instruction* retSite) override
	{
		return Identity<const llvm::Value*>::v();
	}

	map<const llvm::Instruction*, set<const llvm::Value*>> initialSeeds() override
	{
		// just start in main()
		const llvm::Function* mainfunction = icfg.getModule().getFunction("main");
		const llvm::Instruction* firstinst = &(*(mainfunction->begin()->begin()));
		set<const llvm::Value*> iset{zeroValue()};
		map<const llvm::Instruction*, set<const llvm::Value*>> imap{ {firstinst, iset} };
		return imap;
	}

	const llvm::Value* createZeroValue() override
	{
		// create a special value to represent the zero value!
		static llvm::Value* zeroValue = llvm::ConstantInt::get(context, llvm::APInt(0, 0, true));
		return zeroValue;
	}

	// in addition provide specifications for the IDE parts

	shared_ptr<EdgeFunction<const llvm::Value*>> getNormalEdgeFunction(const llvm::Instruction* curr,
																	   const llvm::Value* currNode,
																	   const llvm::Instruction* succ,
																	   const llvm::Value* succNode) override
	{
		return EdgeIdentity<const llvm::Value*>::v();
	}

	shared_ptr<EdgeFunction<const llvm::Value*>> getCallEdgeFunction(const llvm::Instruction* callStmt,
			                                                         const llvm::Value* srcNode,
																	 const llvm::Function* destiantionMethod,
																	 const llvm::Value* destNode) override
	{
		return EdgeIdentity<const llvm::Value*>::v();
	}

	shared_ptr<EdgeFunction<const llvm::Value*>> getReturnEdgeFunction(const llvm::Instruction* callSite,
			                                                           const llvm::Function* calleeMethod,
																	   const llvm::Instruction* exitStmt,
																	   const llvm::Value* exitNode,
																	   const llvm::Instruction* reSite,
																	   const llvm::Value* retNode) override
	{
		return EdgeIdentity<const llvm::Value*>::v();
	}

	shared_ptr<EdgeFunction<const llvm::Value*>> getCallToReturnEdgeFunction(const llvm::Instruction* callSite,
			                                                                 const llvm::Value* callNode,
																			 const llvm::Instruction* retSite,
																			 const llvm::Value* retSiteNode) override
	{
		return EdgeIdentity<const llvm::Value*>::v();
	}

	const llvm::Value* topElement() override
	{
		return nullptr;
	}

	const llvm::Value* bottomElement() override
	{
		return nullptr;
	}

	const llvm::Value* join(const llvm::Value* lhs, const llvm::Value* rhs) override
	{
		return nullptr;
	}

	shared_ptr<EdgeFunction<const llvm::Value*>> allTopFunction() override
	{
		return make_shared<IDETainAnalysisAllTop>();
	}

	class IDETainAnalysisAllTop : public EdgeFunction<const llvm::Value*>,
								  public enable_shared_from_this<IDETainAnalysisAllTop> {
		const llvm::Value* computeTarget(const llvm::Value* source) override
		{
			return nullptr;
		}

		shared_ptr<EdgeFunction<const llvm::Value*>> composeWith(shared_ptr<EdgeFunction<const llvm::Value*>> secondFunction) override
		{
			return EdgeIdentity<const llvm::Value*>::v();
		}

		shared_ptr<EdgeFunction<const llvm::Value*>> joinWith(shared_ptr<EdgeFunction<const llvm::Value*>> otherFunction) override
		{
			return EdgeIdentity<const llvm::Value*>::v();
		}

		bool equalTo(shared_ptr<EdgeFunction<const llvm::Value*>> other) override
		{
			return false;
		}
	};

};

#endif /* ANALYSIS_IFDS_IDE_PROBLEMS_IDE_TAINT_ANALYSIS_IDETAINTANALYSIS_HH_ */
