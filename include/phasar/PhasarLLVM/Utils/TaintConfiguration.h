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
 *  Created on: 12.08.2019
 *      Author: linus jungemann
 */

#ifndef PHASAR_PHASARLLVM_UTILS_TAINTCONFIGURATION_H
#define PHASAR_PHASARLLVM_UTILS_TAINTCONFIGURATION_H

#include <cassert>
#include <fstream>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include "boost/filesystem.hpp"

#include "llvm/Support/ErrorHandling.h"

#include "nlohmann/json.hpp"

#include "phasar/Config/Configuration.h"

namespace llvm {
class Instruction;
} // namespace llvm

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
template <typename D> class TaintConfiguration {
public:
  struct SourceFunction;
  struct SinkFunction;

  struct All;
  struct None;

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
  /// Holds all initial seeds
  std::map<const llvm::Instruction *, std::set<D>> seedMap;

  void importSourceSinkFunctions(const std::string &FilePath) {
    std::cout << "Parsing JSON with source and sink functions\n";
    if (boost::filesystem::exists(FilePath) &&
        !boost::filesystem::is_directory(FilePath)) {
      std::ifstream ifs(FilePath);
      if (ifs.is_open()) {
        std::stringstream iss;
        iss << ifs.rdbuf();
        ifs.close();
        nlohmann::json SSFunctions;
        iss >> SSFunctions;
        std::cout << std::setw(2) << SSFunctions << '\n';
        if (SSFunctions.find(SourceJSONId) != SSFunctions.end()) {
          // Discarding default source functions
          Sources.clear();
          nlohmann::json JSources = SSFunctions.at(SourceJSONId);
          for (auto It = JSources.begin(); It != JSources.end(); ++It) {
            std::string Fname = It.key();
            nlohmann::json JSource = It.value();
            if (JSource.find(ReturnJSONId) != JSource.end() &&
                JSource.find(ArgumentJSONId) != JSource.end()) {
              bool ReturnBool = JSource.at(ReturnJSONId);
              std::vector<unsigned> ArgVec = JSource.at(ArgumentJSONId);
              Sources.insert(
                  make_pair(Fname, TaintConfiguration<D>::SourceFunction(
                                       Fname, ReturnBool, ArgVec)));
            } else {
              throw std::invalid_argument(Fname + " is not valid format!");
            }
          }
        } else {
          std::cout
              << "No Source Functions found. Using default sink functions!\n";
        }
        if (SSFunctions.find(SinkJSONId) != SSFunctions.end()) {
          // Discarding default sink functions
          Sinks.clear();
          nlohmann::json JSinks = SSFunctions.at(SinkJSONId);
          for (auto It = JSinks.begin(); It != JSinks.end(); ++It) {
            std::string Fname = It.key();
            nlohmann::json JSink = It.value();
            if (JSink.find(ArgumentJSONId) != JSink.end()) {
              std::vector<unsigned> ArgVec = JSink.at(ArgumentJSONId);
              Sinks.insert(make_pair(
                  Fname, TaintConfiguration<D>::SinkFunction(Fname, ArgVec)));
            } else {
              throw std::invalid_argument(Fname + " is not valid format!");
            }
          }
        } else {
          std::cout
              << "No sink functions found. Using default sink functions!\n";
        }
      } else {
        throw std::ios_base::failure("Could not open file");
      }
    } else {
      throw std::ios_base::failure(FilePath + " is not a valid path");
    }
  }

public:
  struct All {
    friend bool operator==(const All &Lhs, const All &Rhs) { return true; }
  };
  struct None {
    friend bool operator==(const None &Lhs, const None &Rhs) { return true; }
  };
  /**
   * Encapsulates all taint-relevant information of a source function.
   */
  struct SourceFunction {
    /// Function name.
    std::string Name;
    /// States which function parameter are tainted.
    std::variant<std::vector<unsigned>, TaintConfiguration<D>::All,
                 TaintConfiguration<D>::None>
        TaintedArgs;
    /// States if the function return is tainted.
    bool TaintsReturn;

    SourceFunction(
        std::string FunctionName, bool Ret,
        std::variant<std::vector<unsigned>, TaintConfiguration<D>::All,
                     TaintConfiguration<D>::None>
            Args = TaintConfiguration<D>::All())
        : Name(std::move(FunctionName)), TaintedArgs(std::move(Args)),
          TaintsReturn(Ret){};
    bool isTaintedArg(unsigned ArgIdx) {
      if (auto pval = std::get_if<TaintConfiguration<D>::All>(&TaintedArgs)) {
        return true;
      } else if (auto pval =
                     std::get_if<TaintConfiguration<D>::None>(&TaintedArgs)) {
        return false;
      } else if (auto pval = std::get_if<std::vector<unsigned>>(&TaintedArgs)) {
        return find(pval->begin(), pval->end(), ArgIdx) != pval->end();
      }
      llvm_unreachable("Something went wrong, unexpected type");
      return false;
    }
    friend std::ostream &operator<<(std::ostream &OS,
                                    const SourceFunction &SF) {
      OS << "F: " << SF.Name << " Args: [ ";
      if (auto pval =
              std::get_if<TaintConfiguration<D>::All>(&SF.TaintedArgs)) {
        OS << "All"
           << " ";
      } else if (auto pval = std::get_if<TaintConfiguration<D>::None>(
                     &SF.TaintedArgs)) {
        OS << "None"
           << " ";
      } else if (auto pval =
                     std::get_if<std::vector<unsigned>>(&SF.TaintedArgs)) {
        for (auto Arg : *pval)
          OS << Arg << " ";
      } else {
        llvm_unreachable("Something went wrong, unexpected type");
      }
      return OS << "] Return: " << std::boolalpha << SF.TaintsReturn << "\n";
    }
    friend bool operator==(const SourceFunction &Lhs,
                           const SourceFunction &Rhs) {
      return Lhs.Name == Rhs.Name && Lhs.TaintedArgs == Rhs.TaintedArgs &&
             Lhs.TaintsReturn == Rhs.TaintsReturn;
    }
  };

  /**
   * Encapsulates all taint-relevant information of a sink function.
   */
  struct SinkFunction {
    /// Funciton name.
    std::string Name;
    /// States which function arguments will be leaked.
    std::variant<std::vector<unsigned>, TaintConfiguration<D>::All,
                 TaintConfiguration<D>::None>
        LeakedArgs;

    SinkFunction(std::string FunctionName,
                 std::variant<std::vector<unsigned>, TaintConfiguration<D>::All,
                              TaintConfiguration<D>::None>
                     Args)
        : Name(std::move(FunctionName)), LeakedArgs(std::move(Args)){};
    bool isLeakedArg(unsigned ArgIdx) {
      if (auto pval = std::get_if<TaintConfiguration<D>::All>(&LeakedArgs)) {
        return true;
      } else if (auto pval =
                     std::get_if<TaintConfiguration<D>::None>(&LeakedArgs)) {
        return false;
      } else if (auto pval = std::get_if<std::vector<unsigned>>(&LeakedArgs)) {
        return find(pval->begin(), pval->end(), ArgIdx) != pval->end();
      } else {
        throw std::runtime_error("Something went wrong, unexpected type");
      }
    }
    friend std::ostream &operator<<(std::ostream &OS, const SinkFunction &SF) {
      OS << "F: " << SF.Name << " Args: [ ";
      if (auto pval = std::get_if<TaintConfiguration<D>::All>(&SF.LeakedArgs)) {
        OS << "All"
           << " ";
      } else if (auto pval =
                     std::get_if<TaintConfiguration<D>::None>(&SF.LeakedArgs)) {
        OS << "None"
           << " ";
      } else if (auto pval =
                     std::get_if<std::vector<unsigned>>(&SF.LeakedArgs)) {
        for (auto Arg : *pval)
          OS << Arg << " ";
      } else {
        throw std::runtime_error("Something went wrong, unexpected type");
      }
      return OS << "]\n";
    }
    friend bool operator==(const SinkFunction &Lhs, const SinkFunction &Rhs) {
      return Lhs.Name == Rhs.Name && Lhs.LeakedArgs == Rhs.LeakedArgs;
    }
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
  TaintConfiguration(const std::string &FilePath) {
    importSourceSinkFunctions(FilePath);
  }
  /**
   * @brief Specify functions as sources and sinks
   */
  // clang-format off
  TaintConfiguration(
      std::initializer_list<SourceFunction> SourceFunctions =
          {TaintConfiguration<D>::SourceFunction("fgetc", true),
           TaintConfiguration<D>::SourceFunction("fgets", true,
                                                 std::vector<unsigned>({0})),
           TaintConfiguration<D>::SourceFunction("fread", false,
                                                 std::vector<unsigned>({0})),
           TaintConfiguration<D>::SourceFunction("getc", true),
           TaintConfiguration<D>::SourceFunction("getchar", true),
           TaintConfiguration<D>::SourceFunction("read", false,
                                                 std::vector<unsigned>({1})),
           TaintConfiguration<D>::SourceFunction("ungetc", true)},
      std::initializer_list<SinkFunction> SinkFunctions = {
          TaintConfiguration<D>::SinkFunction("fputc",
                                              std::vector<unsigned>({0})),
          TaintConfiguration<D>::SinkFunction("fputs",
                                              std::vector<unsigned>({0})),
          TaintConfiguration<D>::SinkFunction("fwrite",
                                              std::vector<unsigned>({0})),
          TaintConfiguration<D>::SinkFunction(
              "printf", std::vector<unsigned>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9})),
          TaintConfiguration<D>::SinkFunction("putc",
                                              std::vector<unsigned>({0})),
          TaintConfiguration<D>::SinkFunction("putchar",
                                              std::vector<unsigned>({0})),
          TaintConfiguration<D>::SinkFunction("puts",
                                              std::vector<unsigned>({0})),
          TaintConfiguration<D>::SinkFunction("write",
                                              std::vector<unsigned>({1}))}) {
    for (auto elem : SourceFunctions) {
      Sources.insert(make_pair(elem.Name, elem));
    }
    for (auto elem : SinkFunctions) {
      Sinks.insert(make_pair(elem.Name, elem));
    }
  }
  // clang-format on
  /**
   * @brief Specify instructions as sources and sinks
   */
  TaintConfiguration(
      std::initializer_list<const llvm::Instruction *> SourceInsts,
      std::initializer_list<const llvm::Instruction *> SinkInsts)
      : SourceInstructions(SourceInsts.begin(), SourceInsts.end()),
        SinkInstructions(SinkInsts.begin(), SinkInsts.end()) {}
  /**
   * @brief Specify initial seeds the analysis starts with
   */
  TaintConfiguration(std::map<const llvm::Instruction *, std::set<D>> Seeds)
      : seedMap(Seeds) {}
  ~TaintConfiguration() = default;

  void addInitialSeeds(std::map<const llvm::Instruction *, std::set<D>> Seeds) {
    seedMap = Seeds;
  }
  std::map<const llvm::Instruction *, std::set<D>> getInitialSeeds() {
    return seedMap;
  }
  void addSource(SourceFunction src) {
    Sources.insert(make_pair(src.Name, src));
  }
  void addSink(SinkFunction snk) { Sinks.insert(make_pair(snk.Name, snk)); }
  bool isSource(const std::string &FunctionName) const {
    return Sources.find(FunctionName) != Sources.end();
  }
  bool isSource(const llvm::Instruction *I) const {
    return SourceInstructions.count(I);
  }
  bool isSink(const std::string &FunctionName) const {
    return Sinks.find(FunctionName) != Sinks.end();
  }
  bool isSink(const llvm::Instruction *I) const {
    return SinkInstructions.count(I);
  }
  SourceFunction getSource(const std::string &FunctionName) const {
    return Sources.at(FunctionName);
  }
  SinkFunction getSink(const std::string &FunctionName) const {
    return Sinks.at(FunctionName);
  }

  friend std::ostream &operator<<(std::ostream &OS,
                                  const TaintConfiguration<D> &TSF) {
    OS << "Source Functions:\n";
    for (auto SF : TSF.Sources) {
      OS << SF.second << "\n";
    }
    OS << "Sink Functions:\n";
    for (auto SF : TSF.Sinks)
      OS << SF.second << "\n";
    return OS;
  }
};

} // namespace psr

#endif
