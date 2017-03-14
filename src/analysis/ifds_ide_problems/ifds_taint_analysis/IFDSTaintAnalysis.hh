/*
 * IFDSTaintAnalysis.hh
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */
#ifndef ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TAINT_ANALYSIS_IFDSTAINTANALYSIS_HH_
#define ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TAINT_ANALYSIS_IFDSTAINTANALYSIS_HH_

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
#include "../../ifds_ide/flow_func/Gen.hh"
#include "../../ifds_ide/flow_func/Kill.hh"
#include "../../ifds_ide/DefaultIFDSTabulationProblem.hh"
#include "../../ifds_ide/FlowFunction.hh"
#include "../../ifds_ide/icfg/LLVMBasedInterproceduralCFG.hh"
#include "../../ifds_ide/DefaultSeeds.hh"
#include "../../ifds_ide/flow_func/Identity.hh"
using namespace std;

class IFDSTaintAnalysis : public DefaultIFDSTabulationProblem
										<const llvm::Instruction*,
										 const llvm::Value*,
										 const llvm::Function*,
										 LLVMBasedInterproceduralICFG&
										> {
private:
	llvm::LLVMContext& context;

public:
	struct SourceFunction {
		string name;
		vector<unsigned> genargs;
		bool genreturn;
		SourceFunction() : genreturn(0) {}
		SourceFunction(const SourceFunction& sf) = default;
		SourceFunction(string n, vector<unsigned> gen, bool genret) : name(n), genargs(gen), genreturn(genret) {}
		friend ostream& operator<< (ostream& os, const SourceFunction& sf)
		{
			os << sf.name << "\n";
			for (auto arg : sf.genargs) os << arg << ",";
			return os << "\n" << sf.genreturn << endl;
		}
	};
	struct SinkFunction {
		string name;
		vector<unsigned> sinkargs;
		SinkFunction() {}
		SinkFunction(const SinkFunction& sf) = default;
		SinkFunction(string n, vector<unsigned> sink) : name(n), sinkargs(sink) {}
		friend ostream& operator<< (ostream& os, const SinkFunction& sf)
		{
			os << sf.name << "\n";
			for (auto arg : sf.sinkargs) os << arg << ",";
			return os << endl;
		}
	};

	const vector<SourceFunction> source_functions = { SourceFunction("fread", {0}, false), SourceFunction("read", {1}, false) };
	// keep in mind that 'char** argv' of main is a source for tainted values as well
	const vector<SinkFunction> sink_functions = { SinkFunction("fwrite", {0}), SinkFunction("write", {1}), SinkFunction("printf", {1,2,3,4,5,6,7,8,9,10}) };

	SourceFunction findSourceFunction(const llvm::Function* f)
	{
		for (auto sourcefunction : source_functions) {
				if (f->getName().str().find(sourcefunction.name) != string::npos)
					return sourcefunction;
		}
		return SourceFunction();
	}

	SinkFunction findSinkFunction(const llvm::Function* f)
	{
		for (auto sinkfunction : sink_functions) {
				if (f->getName().str().find(sinkfunction.name) != string::npos)
					return sinkfunction;
		}
		return SinkFunction();
	}

	bool isSourceFunction(const llvm::Function* f)
	{
		for (auto sourcefunction : source_functions) {
			if (f->getName().str().find(sourcefunction.name) != string::npos)
				return true;
		}
		return false;
	}

	bool isSinkFunction(const llvm::Function* f)
	{
		for (auto sinkfunction : sink_functions) {
			if (f->getName().str().find(sinkfunction.name) != string::npos)
				return true;
		}
		return false;
	}


	IFDSTaintAnalysis(LLVMBasedInterproceduralICFG& icfg, llvm::LLVMContext& c) : DefaultIFDSTabulationProblem(icfg), context(c)
	{
		DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
	}

	virtual ~IFDSTaintAnalysis() = default;

	shared_ptr<FlowFunction<const llvm::Value*>> getNormalFlowFunction(const llvm::Instruction* curr,
																	   const llvm::Instruction* succ) override
	{
		cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getNormalFlowFunction()" << endl;
		// Taint the commandline arguments
		if (icfg.getNameOfMethod(curr) == "main" && icfg.isStartPoint(curr)) {
			struct TAFF : FlowFunction<const llvm::Value*> {
				const llvm::Function* function;
				const llvm::Value* zerovalue;
				TAFF(const llvm::Function* f, const llvm::Value* zv) : function(f), zerovalue(zv) {}
					set<const llvm::Value*> computeTargets(const llvm::Value* source) override
					{
						if (source == zerovalue) {
							set<const llvm::Value*> res;
							for (auto& commandline_arg : function->getArgumentList()) {
								res.insert(&commandline_arg);
							}
							res.insert(zerovalue);
							return res;
						} else {
							return set<const llvm::Value*>{source};
						}
					}
			};
			return make_shared<TAFF>(curr->getFunction(), zeroValue());
		}
		// If a tainted value is stored, the store location is also tainted
		if (llvm::isa<llvm::StoreInst>(curr)) {
			const llvm::StoreInst* store = llvm::dyn_cast<llvm::StoreInst>(curr);
			struct TAFF : FlowFunction<const llvm::Value*> {
				const llvm::StoreInst* store;
				TAFF(const llvm::StoreInst* s) : store(s) {};
				set<const llvm::Value*> computeTargets(const llvm::Value* source) override
				{
					if (store->getValueOperand() == source) {
						return set<const llvm::Value*>{ store->getPointerOperand(), source };
					} else if (store->getValueOperand() != source && store->getPointerOperand() == source){
						return set<const llvm::Value*>{ };
					} else {
						return set<const llvm::Value*>{ source };
					}
				}
			};
			return make_shared<TAFF>(store);
		}
		// If a tainted value is loaded, the loaded value is of course tainted
		if (llvm::isa<llvm::LoadInst>(curr)) {
			const llvm::LoadInst* load = llvm::dyn_cast<llvm::LoadInst>(curr);
			struct TAFF : FlowFunction<const llvm::Value*> {
				const llvm::LoadInst* load;
				TAFF(const llvm::LoadInst* l) : load(l) {}
				set<const llvm::Value*> computeTargets(const llvm::Value* source) override
				{
					if (source == load->getPointerOperand()) {
						return set<const llvm::Value*>{ load, source };
					} else {
						return set<const llvm::Value*>{ source };
					}
				}
			};
			return make_shared<TAFF>(load);
		}
		// Otherwise we do not care and leave everything as it is
		return Identity<const llvm::Value*>::v();
	}

	shared_ptr<FlowFunction<const llvm::Value*>> getCallFlowFuntion(const llvm::Instruction* callStmt,
																	const llvm::Function* destMthd) override
	{
		cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getCallFlowFunction()" << endl;
		if (llvm::isa<llvm::CallInst>(callStmt)) {
			const llvm::CallInst* call = llvm::dyn_cast<llvm::CallInst>(callStmt);
			if (isSourceFunction(call->getCalledFunction())) {
				// Generate the values, that are tainted by this call to a source function
				struct TAFF : FlowFunction<const llvm::Value*> {
					const llvm::CallInst* call;
					const llvm::Value* zerovalue;
					const SourceFunction sourcefunction;
					TAFF(const llvm::CallInst* cs, const llvm::Value* zv, const SourceFunction sf) : call(cs), zerovalue(zv), sourcefunction(sf) {}
					set<const llvm::Value*> computeTargets(const llvm::Value* source)
					{
						if (source == zerovalue) {
							set<const llvm::Value*> res;
							for (auto idx : sourcefunction.genargs) {
								res.insert(call->getArgOperand(idx));
							}
							if (sourcefunction.genreturn) {
								for (auto user : call->users()) {
									res.insert(user);
								}
							}
							res.insert(zerovalue);
							return res;
						} else {
							return set<const llvm::Value*>{ source };
						}
					}
				};
				return make_shared<TAFF>(call, zeroValue(), findSourceFunction(call->getCalledFunction()));
			} else if (isSinkFunction(call->getCalledFunction())) {
				struct TAFF : FlowFunction<const llvm::Value*> {
					const llvm::CallInst* call;
					const SinkFunction sinkfunction;
					TAFF(const llvm::CallInst* cs, const SinkFunction sf) : call(cs), sinkfunction(sf) {}
					set<const llvm::Value*> computeTargets(const llvm::Value* source)
					{
						for (auto idx : sinkfunction.sinkargs) {
							if (call->getArgOperand(idx) == source) {
								cout << "Uuups, found a taint!" << endl;
								return set<const llvm::Value*>{ source };
							}
						}
						return set<const llvm::Value*>{ source };
					}
				};
				return make_shared<TAFF>(call, findSinkFunction(call->getCalledFunction()));
			} else {
				// Map the parameters to the 'normal' function
				struct TAFF : FlowFunction<const llvm::Value*> {
					const llvm::CallInst* call;
					const llvm::Value* zerovalue;
					TAFF(const llvm::CallInst* cs, const llvm::Value* zv) : call(cs), zerovalue(zv) {}
					set<const llvm::Value*> computeTargets(const llvm::Value* source)
					{
						// TODO: this needs some work!
						vector<const llvm::Value*> actuals;
						for (unsigned idx = 0; idx < call->getNumArgOperands(); ++idx) {
							actuals.push_back(call->getArgOperand(idx));
						}
						vector<const llvm::Value*> formals;
						for (unsigned idx = 0; idx < call->getCalledFunction()->getNumOperands(); ++idx) {
							formals.push_back(call->getCalledFunction()->getOperandUse(idx));
						}

						set<const llvm::Value*> res;
						for (unsigned idx = 0; idx < actuals.size(); ++idx) {
							if (source == actuals[idx]) {
								res.insert(formals[idx]);
								res.insert(source);
								res.insert(zerovalue);
							}
						}
						return res;


						if (source == zerovalue) {
							unsigned argcounter = 0;
							set<const llvm::Value*> res;
							for (auto& actual : call->arg_operands()) {
								for (auto& formal : call->getCalledFunction()->args()) {
//									if (&actual == source) {
//										res.insert(&formal);
//									}
								}
							}
							res.insert(zerovalue);
							return res;
						}
						return set<const llvm::Value*>{ source };
					}
				};
				return make_shared<TAFF>(call, zeroValue());
				return Identity<const llvm::Value*>::v();
			}
//		} else if (llvm::isa<llvm::InvokeInst>(callStmt)) {
//			// TODO handle invoke statement
//			return Identity<const llvm::Value*>::v();
		}
		return Identity<const llvm::Value*>::v();
	}

	shared_ptr<FlowFunction<const llvm::Value*>> getRetFlowFunction(const llvm::Instruction* callSite,
															  	  	const llvm::Function* calleeMthd,
																	const llvm::Instruction* exitStmt,
																	const llvm::Instruction* retSite) override
	{
		cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getRetFlowFunction()" << endl;
		return Identity<const llvm::Value*>::v();
	}

	shared_ptr<FlowFunction<const llvm::Value*>> getCallToRetFlowFunction(const llvm::Instruction* callSite,
																		  const llvm::Instruction* retSite) override
	{
		cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getCallToRetFlowFunction()" << endl;
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

};

#endif /* ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TAINT_ANALYSIS_IFDSTAINTANALYSIS_HH_ */
