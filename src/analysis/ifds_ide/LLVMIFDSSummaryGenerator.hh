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
 #include "../icfg/ICFG.hh"
 #include "../icfg/LLVMBasedICFG.hh"
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
   LLVMIFDSSummaryGenerator(const llvm::Function *F, I icfg, SummaryGenerationCTXStrategy S)
       : IFDSSummaryGenerator<const llvm::Instruction *,
                              const llvm::Value *,
                              const llvm::Function *,
                              I,
                              ConcreteIFDSTabulationProblem,
                              LLVMIFDSSolver<const llvm::Value*, I>>(F, icfg, S) {}
 
   virtual ~LLVMIFDSSummaryGenerator() = default;
 };
 
 #endif /* SRC_ANALYSIS_IFDS_IDE_LLVMIFDSSUMMARYGENERATOR_HH_ */
 