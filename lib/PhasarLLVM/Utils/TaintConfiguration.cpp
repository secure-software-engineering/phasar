/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <boost/filesystem.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <json.hpp>
#include <phasar/PhasarLLVM/Utils/TaintConfiguration.h>

using namespace std;
using namespace psr;

namespace psr {

using json = nlohmann::json;

ostream &operator<<(ostream &OS, const TaintConfiguration::SourceFunction &SF) {
  OS << "F: " << SF.Name << " Args: [ ";
  for (auto Arg : SF.TaintedArgs)
    OS << Arg << " ";
  return OS << "] Return: " << boolalpha << SF.TaintsReturn << "\n";
}

bool operator==(const TaintConfiguration::SourceFunction &Lhs,
                const TaintConfiguration::SourceFunction &Rhs) {
  return Lhs.Name == Rhs.Name && Lhs.TaintedArgs == Rhs.TaintedArgs &&
         Lhs.TaintsReturn == Rhs.TaintsReturn;
}

bool TaintConfiguration::SourceFunction::isTaintedArg(unsigned ArgIdx) {
  return find(TaintedArgs.begin(), TaintedArgs.end(), ArgIdx) !=
         TaintedArgs.end();
}

ostream &operator<<(ostream &OS, const TaintConfiguration::SinkFunction &SF) {
  OS << "F: " << SF.Name << " Args: [ ";
  for (auto Arg : SF.LeakedArgs)
    OS << Arg << " ";
  return OS << "]\n";
}

bool operator==(const TaintConfiguration::SinkFunction &Lhs,
                const TaintConfiguration::SinkFunction &Rhs) {
  return Lhs.Name == Rhs.Name && Lhs.LeakedArgs == Rhs.LeakedArgs;
}

bool TaintConfiguration::SinkFunction::isLeakedArg(unsigned ArgIdx) {
  return find(LeakedArgs.begin(), LeakedArgs.end(), ArgIdx) != LeakedArgs.end();
}

TaintConfiguration::TaintConfiguration(bool useDummySourceSink) {
  if (useDummySourceSink) {
    Sources.insert(make_pair("source()", SourceFunction("source()", true)));
    Sinks.insert(make_pair("sink(int)", SinkFunction("sink(int)", {0})));
  }
  // Otherwise use default source and sink functions
  else {
    Sources = {
        {"fgetc", TaintConfiguration::SourceFunction("fgetc", true)},
        {"fgets", TaintConfiguration::SourceFunction("fgets", {0}, true)},
        {"fread", TaintConfiguration::SourceFunction("fread", {0}, false)},
        {"getc", TaintConfiguration::SourceFunction("getc", true)},
        {"getchar", TaintConfiguration::SourceFunction("getchar", true)},
        {"read", TaintConfiguration::SourceFunction("read", {1}, false)},
        {"ungetc", TaintConfiguration::SourceFunction("ungetc", true)}};

    Sinks = {{"fputc", TaintConfiguration::SinkFunction("fputc", {0})},
             {"fputs", TaintConfiguration::SinkFunction("fputs", {0})},
             {"fwrite", TaintConfiguration::SinkFunction("fwrite", {0})},
             {"printf", TaintConfiguration::SinkFunction(
                            "printf", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9})},
             {"putc", TaintConfiguration::SinkFunction("putc", {0})},
             {"putchar", TaintConfiguration::SinkFunction("putchar", {0})},
             {"puts", TaintConfiguration::SinkFunction("puts", {0})},
             {"write", TaintConfiguration::SinkFunction("write", {1})}};
  }
}

TaintConfiguration::TaintConfiguration(
    std::initializer_list<const llvm::Instruction *> sourceInst,
    std::initializer_list<const llvm::Instruction *> sinkInst)
    : SourceInstructions(sourceInst.begin(), sourceInst.end()),
      SinkInstructions(sinkInst.begin(), sinkInst.end()) {}

void TaintConfiguration::importSourceSinkFunctions(
    const std::string &FilePath) {
  cout << "Parsing JSON with source and sink functions\n";
  if (boost::filesystem::exists(FilePath) &&
      !boost::filesystem::is_directory(FilePath)) {
    ifstream ifs(FilePath);
    if (ifs.is_open()) {
      stringstream iss;
      iss << ifs.rdbuf();
      ifs.close();
      json SSFunctions;
      iss >> SSFunctions;
      cout << setw(2) << SSFunctions << '\n';
      if (SSFunctions.find(SourceJSONId) != SSFunctions.end()) {
        // Discarding default source functions
        Sources.clear();
        json JSources = SSFunctions.at(SourceJSONId);
        for (auto It = JSources.begin(); It != JSources.end(); ++It) {
          string Fname = It.key();
          json JSource = It.value();
          if (JSource.find(ReturnJSONId) != JSource.end() &&
              JSource.find(ArgumentJSONId) != JSource.end()) {
            bool ReturnBool = JSource.at(ReturnJSONId);
            vector<unsigned> ArgVec = JSource.at(ArgumentJSONId);
            Sources.insert(make_pair(Fname, TaintConfiguration::SourceFunction(
                                                Fname, ArgVec, ReturnBool)));
          } else {
            throw invalid_argument(Fname + " is not valid format!");
          }
        }
      } else {
        cout << "No Source Functions found. Using default sink functions!\n";
      }
      if (SSFunctions.find(SinkJSONId) != SSFunctions.end()) {
        // Discarding default sink functions
        Sinks.clear();
        json JSinks = SSFunctions.at(SinkJSONId);
        for (auto It = JSinks.begin(); It != JSinks.end(); ++It) {
          string Fname = It.key();
          json JSink = It.value();
          if (JSink.find(ArgumentJSONId) != JSink.end()) {
            vector<unsigned> ArgVec = JSink.at(ArgumentJSONId);
            Sinks.insert(make_pair(
                Fname, TaintConfiguration::SinkFunction(Fname, ArgVec)));
          } else {
            throw invalid_argument(Fname + " is not valid format!");
          }
        }
      } else {
        cout << "No sink functions found. Using default sink functions!\n";
      }
    } else {
      throw ios_base::failure("Could not open file");
    }
  } else {
    throw ios_base::failure(FilePath + " is not a valid path");
  }
}

bool TaintConfiguration::isSource(const std::string &FunctionName) const {
  return Sources.find(FunctionName) != Sources.end();
}

bool TaintConfiguration::isSource(const llvm::Instruction *I) const {
  return SourceInstructions.count(I);
}

bool TaintConfiguration::isSink(const std::string &FunctionName) const {
  return Sinks.find(FunctionName) != Sinks.end();
}

bool TaintConfiguration::isSink(const llvm::Instruction *I) const {
  return SinkInstructions.count(I);
}

TaintConfiguration::SourceFunction
TaintConfiguration::getSource(const std::string &FunctionName) {
  return Sources.at(FunctionName);
}

TaintConfiguration::SinkFunction
TaintConfiguration::getSink(const std::string &FunctionName) {
  return Sinks.at(FunctionName);
}

std::ostream &operator<<(std::ostream &OS, const TaintConfiguration &TSF) {
  OS << "Source Functions:\n";
  for (auto SF : TSF.Sources) {
    OS << SF.second << "\n";
  }
  OS << "Sink Functions:\n";
  for (auto SF : TSF.Sinks)
    OS << SF.second << "\n";
  return OS;
}

} // namespace psr