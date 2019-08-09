/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * TaintConfiguration.h
 *
 *  Created on: 25.08.2018
 *      Author: richard leer
 */

#ifndef PHASAR_PHASARLLVM_UTILS_TAINTCONFIGURATION_H
#define PHASAR_PHASARLLVM_UTILS_TAINTCONFIGURATION_H

#include <initializer_list>
#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <phasar/Config/Configuration.h>

namespace llvm {
class Instruction;
}  // namespace llvm

namespace psr {
// clang-format off
/**
 * The following functions are considered to as taint-sensitve functions by default:
 *
 * Source functions| Critical argument(s) | Signature
 * ----------------|----------------------|-----------
 * fgetc           | ret                  | int fgetc(FILE *stream)
 * fgets           | 0, ret               | char *fgets(char *s, int size, FILE *stream)
 * fread           | 0                    | size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
 * getc            | ret                  | int getc(FILE *stream)
 * getchar         | ret                  | int getchar(void)
 * read            | 1                    | size_t read(int fd, void *buf, size_t count)
 * ungetc          | ret                  | int ungetc(int c, FILE *stream)
 *
 * <br>
 * Sink functions| Critical argument(s) | Signature
 * --------------|----------------------|-----------
 * fputc         | 0                    | int fputc(int c, FILE *stream)
 * fputs         | 0                    | int fputs(const char *s, FILE *stream)
 * fwrite        | 0                    | size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
 * printf        | all                  | int printf(const char *format, ...)
 * putc          | 0                    | int putc(int c, FILE *stream)
 * putchar       | 0                    | int putchar(int c)
 * puts          | 0                    | int puts(const char *s)
 * write         | 1                    | size_t write(int fd, const void *buf, size_t count)
 *
 * User specified source and sink functions can be imported as JSON.
 *
 * @brief Holds all taint-relevant source and sink functions.
 */  // clang-format on
class TaintConfiguration {

class SourceFunction;
class SinkFunction;

 private:
  // Object id's for parsing JSON
  const std::string SourceJSONId = "Source Functions";
  const std::string SinkJSONId = "Sink Functions";
  const std::string ArgumentJSONId = "Args";
  const std::string ReturnJSONId = "Return";
  /// Holds all source functions
  std::map<std::string, SourceFunction> Sources;
  /// Holds all source functions
  std::map<std::string, SinkFunction> Sinks;
  /// Holds all source instructions
  std::set<const llvm::Instruction *> SourceInstructions;
  /// Holds all sink instruction
  std::set<const llvm::Instruction *> SinkInstructions;

  void TaintConfiguration::importSourceSinkFunctions(
    const std::string &FilePath);

 public:
  /**
   * Encapsulates all taint-relevant information of a source function.
   */
  struct SourceFunction {
    /// Function name.
    std::string Name;
    /// States which function parameter are tainted.
    std::vector<unsigned> TaintedArgs;
    /// States if the function return is tainted.
    bool TaintsReturn;

    SourceFunction(std::string FunctionName, std::vector<unsigned> Args,
                   bool Ret)
        : Name(std::move(FunctionName)),
          TaintedArgs(std::move(Args)),
          TaintsReturn(Ret){};
    SourceFunction(std::string FunctionName, bool Ret)
        : Name(std::move(FunctionName)), TaintsReturn(Ret){};
    bool isTaintedArg(unsigned ArgIdx);
    friend std::ostream &operator<<(std::ostream &OS, const SourceFunction &SF);
    friend bool operator==(const SourceFunction &Lhs,
                           const SourceFunction &Rhs);
  };

  /**
   * Encapsulates all taint-relevant information of a sink function.
   */
  struct SinkFunction {
    /// Funciton name.
    std::string Name;
    /// States which function arguments will be leaked.
    std::vector<unsigned> LeakedArgs;

    SinkFunction(std::string FunctionName, std::vector<unsigned> Args)
        : Name(std::move(FunctionName)), LeakedArgs(std::move(Args)){};
    bool isLeakedArg(unsigned ArgIdx);
    friend std::ostream &operator<<(std::ostream &OS, const SinkFunction &SF);
    friend bool operator==(const SinkFunction &Lhs, const SinkFunction &Rhs);
  };

  /**
   * Source and sink functions have to be in JSON format. An template file can
   * be found in the config/ directory. Note that C++ source/sink function
   * name's have to be demangled.
   *
   * @brief Allows to import user specified source and sink functions.
   * @param FilePath path to JSON file holdind source and sink function
   * definitions.
   */
  TaintConfiguration(const std::string &FilePath);
  /**
   * @brief Specify functions as sources and sinks
   */
  TaintConfiguration(std::initializer_list<SourceFunction> SourceFunctions,
                     std::initializer_list<SinkFunction> SinkFunctions);
  /**
   * @brief Specify instructions as sources and sinks
   */
  TaintConfiguration(
      std::initializer_list<const llvm::Instruction *> SourceInsts,
      std::initializer_list<const llvm::Instruction *> SinkInsts);
  /**
   * @brief Specify initial seeds the analysis starts with
   */
  // TaintConfiguration(std::map<const llvm::Instruction *, std::set<D>> Seeds);
  ~TaintConfiguration() = default;

  bool isSource(const std::string &FunctionName) const;
  bool isSource(const llvm::Instruction *I) const;
  bool isSink(const std::string &FunctionName) const;
  bool isSink(const llvm::Instruction *I) const;
  SourceFunction getSource(const std::string &FunctionName);
  SinkFunction getSink(const std::string &FunctionName);
  friend std::ostream &operator<<(std::ostream &OS,
                                  const TaintConfiguration &TSF);
};

}  // namespace psr

#endif
