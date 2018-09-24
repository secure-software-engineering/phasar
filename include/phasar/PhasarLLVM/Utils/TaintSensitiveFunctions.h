/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * TaintSensitiveFunctions.h
 *
 *  Created on: 25.08.2018
 *      Author: richard leer
 */

#ifndef PHASAR_PHASARLLVM_UTILS_TAINTSENSITIVEFUNCTIONS_H
#define PHASAR_PHASARLLVM_UTILS_TAINTSENSITIVEFUNCTIONS_H

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include <phasar/Config/Configuration.h>

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
 */ // clang-format on
class TaintSensitiveFunctions {
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
        : Name(std::move(FunctionName)), TaintedArgs(std::move(Args)),
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

private:
  // Object id's for parsing JSON
  std::string SourceJSONId = "Source Functions";
  std::string SinkJSONId = "Sink Functions";
  std::string ArgumentJSONId = "Args";
  std::string ReturnJSONId = "Return";

public:
  /// Holds all source functions
  std::map<std::string, SourceFunction> Sources;
  /// Holds all source functions
  std::map<std::string, SinkFunction> Sinks;

  /**
   * The dummy function have the following signature:
   * - int source()
   * - void sink(int)
   *
   * @brief Initializes default source and sink functions, or uses dummy source
   * and sink functions.
   */
  TaintSensitiveFunctions(bool useDummySourceSink = false);
  ~TaintSensitiveFunctions() = default;

  bool isSource(const std::string &FunctionName) const;
  bool isSink(const std::string &FunctionName) const;
  SourceFunction getSource(const std::string &FunctionName);
  SinkFunction getSink(const std::string &FunctionName);
  friend std::ostream &operator<<(std::ostream &OS,
                                  const TaintSensitiveFunctions &TSF);

  /**
   * Source and sink functions have to be in JSON format. An template file can
   * be found in the config/ directory. Note that C++ source/sink function
   * name's have to be demangled.
   *
   * @brief Allows to import user specified source and sink functions.
   * @param FilePath path to JSON file holdind source and sink function
   * definitions.
   */
  void importSourceSinkFunctions(
      const std::string &FilePath = DefaultSourceSinkFunctionsPath);
};

} // namespace psr

#endif
