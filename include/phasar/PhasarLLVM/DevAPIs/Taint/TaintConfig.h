/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DEVAPIS_TAINT_TAINT_CONFIG_H_
#define PHASAR_PHASARLLVM_DEVAPIS_TAINT_TAINT_CONFIG_H_

#include <map>
#include <set>
#include <string>
#include <vector>

namespace llvm {
class Function;
class Instruction;
class Value;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;
class ProjectIRDB;

namespace taintapi {

enum class TaintType { Source, Sink, Sanitizer };

std::string to_string(TaintType Type);

class TaintAPIFunctionInfo {
public:
  TaintType getTaintType() const;
  bool isSource() const;
  bool isSink() const;
  bool isSanitizer() const;
  bool isCompleteFun() const;
  bool hasInterestingReturnValue() const;
  const llvm::Function *getFunction() const;
  std::vector<const llvm::Value *> getParams() const;

  friend std::ostream &operator<<(std::ostream &OS,
                                  const TaintAPIFunctionInfo &FI);

  TaintAPIFunctionInfo(TaintType TT, const llvm::Function *FN, bool CF, bool RV,
                       std::vector<const llvm::Value *> Params);

private:
  TaintType Type;
  const llvm::Function *Fun;
  bool CompleteFun;
  bool RetVal;
  std::vector<const llvm::Value *> Params;
};

// Check if the function call belongs to Phasar Taint API
bool isCallToPhasarTaintConfigurationAPI(const llvm::Instruction *I);

// Check if a variable is declared as Source
bool declaresVarAsSource(const llvm::Instruction *I);

// Check if a variable is declared as Sink
bool declaresVarAsSink(const llvm::Instruction *I);

// Check if a variable is declared as Sanitized
bool declaresVarAsSanitized(const llvm::Instruction *I);

// Check if a Function is declared as Source
bool declaresFunAsSource(const llvm::Instruction *I);

// Check if a Function is declared as Sink
bool declaresFunAsSink(const llvm::Instruction *I);

// Check if a Function is declared as Sanitizer
bool declaresFunAsSanitizer(const llvm::Instruction *I);

// Return variable name declared as source
const llvm::Value *getVarDeclaredAsSource(const llvm::Instruction *I);

// Return variable name declared as sink
const llvm::Value *getVarDeclaredAsSink(const llvm::Instruction *I);

// Return variable name declared as sanitized
const llvm::Value *getVarDeclaredAsSanitized(const llvm::Instruction *I);

// Return data for Functions declared as Source/Sink/Sanitizer
TaintAPIFunctionInfo getAPIFunctionInfo(const llvm::Instruction *I);

std::map<const llvm::Instruction *, std::set<const llvm::Value *>>
makeInitialSeedFromSourceDeclarations(const ProjectIRDB &IRDB,
                                      const LLVMBasedICFG &ICF);

} // namespace taintapi
} // namespace psr

#endif
