/*
 * IFDSTaintAnalysis.hh
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */
#ifndef ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_UNINITIALIZED_VARIABLES_IFDSUNINITIALIZEDVARIABLES_HH_
#define ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_UNINITIALIZED_VARIABLES_IFDSUNINITIALIZEDVARIABLES_HH_

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/LLVMContext.h>
#include <map>
#include <set>
#include <memory>
#include "../../ifds_ide/DefaultIFDSTabulationProblem.hh"
#include "../../ifds_ide/FlowFunction.hh"
#include "../../ifds_ide/icfg/LLVMBasedInterproceduralCFG.hh"
#include "../../ifds_ide/DefaultSeeds.hh"
#include "../../ifds_ide/flow_func/Identity.hh"
#include "../../ifds_ide/flow_func/KillAll.hh"
#include "../../ifds_ide/flow_func/Kill.hh"
#include "../../ifds_ide/flow_func/Gen.hh"
using namespace std;

class IFDSUnitializedVariables : public DefaultIFDSTabulationProblem
										<const llvm::Instruction*,
										 const llvm::Value*,
										 const llvm::Function*,
										 LLVMBasedInterproceduralICFG&
										> {
private:
	llvm::LLVMContext& context;

public:
	IFDSUnitializedVariables(LLVMBasedInterproceduralICFG& icfg, llvm::LLVMContext& c) : DefaultIFDSTabulationProblem(icfg), context(c)
	{
		DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
	}

	virtual ~IFDSUnitializedVariables() = default;

	shared_ptr<FlowFunction<const llvm::Value*>> getNormalFlowFunction(const llvm::Instruction* curr,
																	   const llvm::Instruction* succ) override
	{
		cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getNormalFlowFunction()" << endl;
		// taint every local variable, that is not a function parameter
		if (icfg.getNameOfMethod(curr) == "main" && icfg.isStartPoint(curr)) {
			const llvm::Function* func = icfg.getMethodOf(curr);
			struct UVFF : FlowFunction<const llvm::Value*> {
				const llvm::Function* func;
				const llvm::Value* zerovalue;
				LLVMBasedInterproceduralICFG& icfg;
				UVFF(const llvm::Function* f, const llvm::Value* zv, LLVMBasedInterproceduralICFG& g) : func(f), zerovalue(zv), icfg(g) {}
				set<const llvm::Value*> computeTargets(const llvm::Value* source)
				{
					if (source == zerovalue) {
						set<const llvm::Value*> res;
						// first add all local values
						for (auto inst : icfg.getAllInstructionsOfFunction(func)) {
							if (llvm::isa<llvm::AllocaInst>(inst)) {
								const llvm::AllocaInst* alloc = llvm::dyn_cast<const llvm::AllocaInst>(inst);
								// check if the allocated value is of a primitive type
								auto type = alloc->getAllocatedType();
								if (type->isIntegerTy() || type->isFloatingPointTy() || type->isPointerTy() || type->isArrayTy()) {
									res.insert(alloc);
								}
							}
						}
						// now remove those values that are obtained by function parameters of the entry function
						for (auto& arg : func->getArgumentList()) {
							for (auto user : arg.users()) {
								if (llvm::isa<llvm::StoreInst>(user)) {
									auto store = llvm::dyn_cast<llvm::StoreInst>(user);
									res.erase(store->getPointerOperand());
								}
							}
						}
						res.insert(zerovalue);
						return res;
					}
					return set<const llvm::Value*>{};
				}
			};
			return make_shared<UVFF>(func, zerovalue, icfg);
		}
		// check the store instructions
		if (llvm::isa<llvm::StoreInst>(curr)) {
			const llvm::StoreInst* store = llvm::dyn_cast<const llvm::StoreInst>(curr);
			const llvm::Value* valueop = store->getValueOperand();
			const llvm::Value* pointerop = store->getPointerOperand();
			struct UVFF : FlowFunction<const llvm::Value*> {
				const llvm::Value* valueop;
				const llvm::Value* pointerop;
				UVFF(const llvm::Value* vop, const llvm::Value* pop) : valueop(vop), pointerop(pop) {}
				set<const llvm::Value*> computeTargets(const llvm::Value* source) {
					// check if an uninitialized value is loaded and stored in a variable, then the variable is uninitialized!
					for (auto& use : valueop->uses()) {
						if (llvm::isa<llvm::LoadInst>(use)) {
							const llvm::LoadInst* load = llvm::dyn_cast<llvm::LoadInst>(use);
							// if the following is uninit, then this store must be uninit as well!!!
							if (load->getPointerOperand() == source) {
								return set<const llvm::Value*>{ source, pointerop };
							}
						}
					}
					// otherwise the value is initialized through this store and thus can be killed
					if (pointerop == source) {
						return set<const llvm::Value*>{};
					} else {
						return set<const llvm::Value*>{source};
					}
				}
			};
			return make_shared<UVFF>(valueop, pointerop);
		}
		// otherwise we do not care and nothing changes
		return Identity<const llvm::Value*>::v();
	}

	shared_ptr<FlowFunction<const llvm::Value*>> getCallFlowFuntion(const llvm::Instruction* callStmt,
			                                                        const llvm::Function* destMthd) override
	{
		cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getCallFlowFunction()" << endl;
		// check for a usual function call
		if (llvm::isa<llvm::CallInst>(callStmt)) {
			const llvm::CallInst* call = llvm::dyn_cast<llvm::CallInst>(callStmt);
			cout << "found call to " << call->getCalledFunction()->getName().str() << endl;
			vector<const llvm::Value*> actuals;
			unsigned num_args = call->getNumArgOperands();
			for (unsigned i = 0; i < num_args; ++i) {
				const llvm::Value* val = call->getArgOperand(i);
				// collect all non-global actual parameters
				for (auto& use : val->uses()) {
					if (llvm::isa<llvm::LoadInst>(use)) {
						const llvm::LoadInst* load = llvm::dyn_cast<llvm::LoadInst>(use);
						if (!llvm::isa<llvm::GlobalValue>(load->getPointerOperand())) {
							actuals.push_back(val);
						} else {
							actuals.push_back(nullptr); // mark a global variable
						}
					}
				}
			}

			for (auto a : actuals) {
				if (a)
					a->dump();
				else
					cout << "nullptr" << endl;
			}

			struct UVFF : FlowFunction<const llvm::Value*> {
				const llvm::Function* destMthd;
				vector<const llvm::Value*> actuals;
				const llvm::Value* zerovalue;
				LLVMBasedInterproceduralICFG& icfg;
				UVFF(const llvm::Function* dm, vector<const llvm::Value*> atl, const llvm::Value* zv, LLVMBasedInterproceduralICFG& g) : destMthd(dm), actuals(atl), zerovalue(zv), icfg(g) {}
				set<const llvm::Value*> computeTargets(const llvm::Value* source) override
				{
					for (const llvm::Value* arg : actuals) {
						// do the mapping
						if (!arg) continue; // arg might be nullptr in case of global variable
						for (auto& use : arg->uses()) {
							if (llvm::isa<llvm::LoadInst>(use)) {
								const llvm::LoadInst* load = llvm::dyn_cast<llvm::LoadInst>(use);
								if (source == load->getPointerOperand()) {
									// TODO do not make it overly complicated, if a uninit value is loaded, the load is uninit as well!
									unsigned counter = 0;
									for (auto thing : actuals) {
										if (thing == source)
											break;
										else counter++;
									}
									cout << "right before" << endl;
									source->dump();
									cout << counter << endl;
									for (auto& formalarg : destMthd->getArgumentList()) {
										if (formalarg.getArgNo() == 0) {
											formalarg.dump();
											formalarg.users().begin()->dump();
											return set<const llvm::Value*>{ &formalarg };
										}
									}
								}
							}
						}
					}
					if (source == zerovalue) {
						// gen all locals that are not parameter locals!!!
						// make a set of all uninitialized local variables!
						set<const llvm::Value*> uninitlocals;
						for (auto inst : icfg.getAllInstructionsOfFunction(destMthd)) {
							if (llvm::isa<llvm::AllocaInst>(inst)) {
								const llvm::AllocaInst* alloc = llvm::dyn_cast<const llvm::AllocaInst>(inst);
								// check if the allocated value is of a primitive type
								auto type = alloc->getAllocatedType();
								if (type->isIntegerTy() || type->isFloatingPointTy() || type->isPointerTy() || type->isArrayTy()) {
									uninitlocals.insert(alloc);
								}
							}
						}
						// remove all local variables, that are formal parameters!
						for (auto& arg : destMthd->getArgumentList()) {
							for (auto user : arg.users()) {
								if (llvm::isa<llvm::StoreInst>(user)) {
									auto store = llvm::dyn_cast<llvm::StoreInst>(user);
									uninitlocals.erase(store->getPointerOperand());
								}
							}
						}
						cout << "uninitlocals" << endl;
						for (auto ulocal : uninitlocals)
							ulocal->dump();
						return uninitlocals;
					}
					return set<const llvm::Value*>{};
				}
			};
			return make_shared<UVFF>(destMthd, actuals, zerovalue, icfg);
		} else if (llvm::dyn_cast<llvm::InvokeInst>(callStmt)) {
			/*
			 * TODO consider an invoke statement
			 * An invoke statement must be treated the same as an ordinary call statement
			 */
			return Identity<const llvm::Value*>::v();
		}
		return Identity<const llvm::Value*>::v();
	}

	shared_ptr<FlowFunction<const llvm::Value*>> getRetFlowFunction(const llvm::Instruction* callSite,
			                                                        const llvm::Function* calleeMthd,
			                                                        const llvm::Instruction* exitStmt,
			                                                        const llvm::Instruction* retSite) override
	{
		cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getRetFlowFunction()" << endl;
//		// consider it a value gets store at the call site:
//		// int x = call(...);
//		// x shall be uninitialized then
//		// check if callSite is usual call instruction
		const llvm::CallInst* call = llvm::dyn_cast<llvm::CallInst>(callSite);
		// check if exitStmt is return statement
		for (auto user : call->users()) {
			if (llvm::isa<llvm::StoreInst>(user)) {
				const llvm::StoreInst* store = llvm::dyn_cast<llvm::StoreInst>(user);
				struct UVFF : public FlowFunction<const llvm::Value*> {
					const llvm::StoreInst* store;
					UVFF(const llvm::StoreInst* s) : store(s) {}
					set<const llvm::Value*> computeTargets(const llvm::Value* source) override
					{
						if (store->getPointerOperand() == source)
							return set<const llvm::Value*>{store->getPointerOperand()};
						return set<const llvm::Value*>{};
					}
				};
				return make_shared<UVFF>(store);
			}
		}

//		// TODO: not quite - check ICFG implementation!
//		// check if callSite is invoke instruction
//		if (llvm::isa<llvm::InvokeInst>(exitStmt)) {
//			const llvm::InvokeInst* invoke = llvm::dyn_cast<llvm::InvokeInst>(exitStmt);
//			for (auto user : invoke->users()) {
//				if (llvm::isa<llvm::StoreInst>(user)) {
//					const llvm::StoreInst* store = llvm::dyn_cast<llvm::StoreInst>(user);
//					return make_shared<Gen<const llvm::Value*>>(store->getPointerOperand(), zerovalue);
//				}
//			}
//		}
		/*
		 *TODO check, this does not seem to be right, should it not be id, so we can check at methods return statement,
		 *TODO otherwise we must check one statement before return statement.
		 */
		return KillAll<const llvm::Value*>::v();
		// just a test
//		return Identity<const llvm::Value*>::v();
	}

	shared_ptr<FlowFunction<const llvm::Value*>> getCallToRetFlowFunction(const llvm::Instruction* callSite,
			                                                              const llvm::Instruction* retSite) override
	{
		cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% getCallToRetFlowFunction()" << endl;
		for (auto user : callSite->users()) {
			if (llvm::isa<llvm::StoreInst>(user)) {
				const llvm::StoreInst* store = llvm::dyn_cast<llvm::StoreInst>(user);
				return make_shared<Kill<const llvm::Value*>>(store->getPointerOperand());
			}
		}
		return Identity<const llvm::Value*>::v();
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
		// create a special value to represent the zero value!
		static llvm::Value* zeroValue = llvm::ConstantInt::get(context, llvm::APInt(0, 0, true));
		return zeroValue;
	}

};

#endif /* ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TAINT_ANALYSIS_IFDSTAINTANALYSIS_HH_ */
