/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IFDSTAINTANALYSIS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IFDSTAINTANALYSIS_H_

#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <phasar/PhasarLLVM/IfdsIde/DefaultIFDSTabulationProblem.h>

// Forward declaration of types for which we only use its pointer or ref type
namespace llvm {
class Instruction;
class Function;
class Value;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;

// clang-format off
/**
 * This analysis tracks data-flows through a program. Data flows from
 * dedicated source functions, which generate tainted values, into
 * dedicated sink functions. A leak is reported once a tainted value
 * reached a sink function.
 *
 * Functions that are considered as taint-sensitve functions:
 *
 * Source functions| critical argument(s) | signature
 * -----------------------------------------------------
 * fgetc   | ret | int fgetc(FILE *stream);
 * fgets   |0,ret| char *fgets(char *s, int size, FILE *stream);
 * fread   | 0   | size_t fread(void *ptr, size_t size,
 *                              size_t nmemb, FILE *stream);
 * getc    | ret | int getc(FILE *stream);
 * getchar | ret | int getchar(void);
 * read    | 1   | size_t read(int fd, void *buf, size_t count);
 * ungetc  | ret | int ungetc(int c, FILE *stream);
 *
 * Sink functions| critical argument(s) | signature
 * -----------------------------------------------------
 * fputc   | 0   | int fputc(int c, FILE *stream);
 * fputs   | 0   | int fputs(const char *s, FILE *stream);
 * fwrite  | 0   | size_t fwrite(const void *ptr, size_t size,
 *                               size_t nmemb, FILE *stream);
 * printf  | all | int printf(const char *format, ...);
 * putc    | 0   | int putc(int c, FILE *stream);
 * putchar | 0   | int putchar(int c);
 * puts    | 0   | int puts(const char *s);
 * write   | 1   | size_t write(int fd, const void *buf, size_t count);
 */ // clang-format on
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
    friend std::ostream &operator<<(std::ostream &os, const SourceFunction &sf);
  };

  struct SinkFunction {
    std::string name;
    std::vector<unsigned> sinkargs;
    SinkFunction(std::string n, std::vector<unsigned> sink)
        : name(n), sinkargs(sink) {}
    friend std::ostream &operator<<(std::ostream &os, const SinkFunction &sf);
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
  getCallToRetFlowFunction(n_t callSite, n_t retSite,
                           std::set<m_t> callees) override;

  std::shared_ptr<FlowFunction<d_t>>
  getSummaryFlowFunction(n_t callStmt, m_t destMthd) override;

  std::map<n_t, std::set<d_t>> initialSeeds() override;

  d_t createZeroValue() override;

  bool isZeroValue(d_t d) const override;

  std::string DtoString(d_t d) const override;

  std::string NtoString(n_t n) const override;

  std::string MtoString(m_t m) const override;

  void printLeaks() const;
};
} // namespace psr

#endif
