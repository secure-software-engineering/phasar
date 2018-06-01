/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TAINTANALYSIS_H_
#define ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TAINTANALYSIS_H_

#include <map>
#include <phasar/PhasarLLVM/IfdsIde/DefaultIFDSTabulationProblem.h>
#include <set>
#include <string>
#include <vector>

// Forward declaration of types for which we only use its pointer or ref type
namespace llvm {
class Instruction;
class Function;
class Value;
} // namespace llvm

class LLVMBasedICFG;

// Functions that are considered as taint sensitve functions:
//
// Source functions - critical argument(s):
//  -fread - 0
//  -read - 1
//  -fgetc - ret
//  -fgets - 0, ret
//  -getc - ret
//  -getchar - ret
//  -ungetc - ret
//
// Sink functions:
//  -fwrite - 0
//  -write - 1
//  -printf - everything
//  -fputc - 0
//  -fputs - 0
//  -putc - 0
//  -putchar - 0
//  -puts - 0
class IFDSTaintAnalysis : public DefaultIFDSTabulationProblem<
                              const llvm::Instruction *, const llvm::Value *,
                              const llvm::Function *, LLVMBasedICFG &> {
private:
  std::vector<std::string> EntryPoints;

public:
  typedef const llvm::Value *d_t;
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Function *m_t;
  typedef LLVMBasedICFG &i_t;

  struct SourceFunction {
    std::string name;
    std::vector<unsigned> genargs;
    bool genreturn;
    SourceFunction(std::string n, std::vector<unsigned> gen, bool genret)
        : name(n), genargs(gen), genreturn(genret) {}
    SourceFunction(std::string n, bool genret) : name(n), genreturn(genret) {}
    friend std::ostream &operator<<(std::ostream &os,
                                    const SourceFunction &sf) {
      os << sf.name << ": ";
      for (auto arg : sf.genargs)
        os << arg << ",";
      return os << ", " << sf.genreturn << std::endl;
    }
  };

  struct SinkFunction {
    std::string name;
    std::vector<unsigned> sinkargs;
    SinkFunction(std::string n, std::vector<unsigned> sink)
        : name(n), sinkargs(sink) {}
    friend std::ostream &operator<<(std::ostream &os, const SinkFunction &sf) {
      os << sf.name << ": ";
      for (auto arg : sf.sinkargs)
        os << arg << ",";
      return os << std::endl;
    }
  };

  std::map<n_t, std::set<d_t>> Leaks;

  static const std::map<std::string, SourceFunction> Sources;

  static const std::map<std::string, SinkFunction> Sinks;

  IFDSTaintAnalysis(i_t icfg, std::vector<std::string> EntryPoints = {"main"});

  virtual ~IFDSTaintAnalysis() = default;

  std::shared_ptr<FlowFunction<d_t>> getNormalFlowFunction(n_t curr,
                                                           n_t succ) override;

  std::shared_ptr<FlowFunction<d_t>> getCallFlowFunction(n_t callStmt,
                                                         m_t destMthd) override;

  std::shared_ptr<FlowFunction<d_t>> getRetFlowFunction(n_t callSite,
                                                        m_t calleeMthd,
                                                        n_t exitStmt,
                                                        n_t retSite) override;

  std::shared_ptr<FlowFunction<d_t>>
  getCallToRetFlowFunction(n_t callSite, n_t retSite, std::set<m_t> callees) override;

  std::shared_ptr<FlowFunction<d_t>>
  getSummaryFlowFunction(n_t callStmt, m_t destMthd) override;

  std::map<n_t, std::set<d_t>> initialSeeds() override;

  d_t createZeroValue() override;

  bool isZeroValue(d_t d) const override;

  std::string DtoString(d_t d) const override;

  std::string NtoString(n_t n) const override;

  std::string MtoString(m_t m) const override;
};

#endif /* ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TAINTANALYSIS_H_ */
