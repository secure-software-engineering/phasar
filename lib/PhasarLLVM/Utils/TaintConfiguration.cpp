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

bool operator==(const TaintConfiguration::All &Lhs,
                const TaintConfiguration::All &Rhs) {
  return true;
}

bool operator==(const TaintConfiguration::None &Lhs,
                const TaintConfiguration::None &Rhs) {
  return true;
}

ostream &operator<<(ostream &OS, const TaintConfiguration::SourceFunction &SF) {
  OS << "F: " << SF.Name << " Args: [ ";
  if (auto pval = std::get_if<TaintConfiguration::All>(&SF.TaintedArgs)) {
    OS << "All"
       << " ";
  } else if (auto pval =
                 std::get_if<TaintConfiguration::None>(&SF.TaintedArgs)) {
    OS << "None"
       << " ";
  } else if (auto pval = std::get_if<std::vector<unsigned>>(&SF.TaintedArgs)) {
    for (auto Arg : *pval)
      OS << Arg << " ";
  } else {
    throw std::runtime_error("Something went wrong, unexpected type");
  }

  return OS << "] Return: " << boolalpha << SF.TaintsReturn << "\n";
}

bool operator==(const TaintConfiguration::SourceFunction &Lhs,
                const TaintConfiguration::SourceFunction &Rhs) {
  return Lhs.Name == Rhs.Name && Lhs.TaintedArgs == Rhs.TaintedArgs &&
         Lhs.TaintsReturn == Rhs.TaintsReturn;
}

bool TaintConfiguration::SourceFunction::isTaintedArg(unsigned ArgIdx) {
  if (auto pval = std::get_if<TaintConfiguration::All>(&TaintedArgs)) {
    return true;
  } else if (auto pval = std::get_if<TaintConfiguration::None>(&TaintedArgs)) {
    return false;
  } else if (auto pval = std::get_if<std::vector<unsigned>>(&TaintedArgs)) {
    return find(pval->begin(), pval->end(), ArgIdx) != pval->end();
  } else {
    throw std::runtime_error("Something went wrong, unexpected type");
  }
}

ostream &operator<<(ostream &OS, const TaintConfiguration::SinkFunction &SF) {
  OS << "F: " << SF.Name << " Args: [ ";
  if (auto pval = std::get_if<TaintConfiguration::All>(&SF.LeakedArgs)) {
    OS << "All"
       << " ";
  } else if (auto pval =
                 std::get_if<TaintConfiguration::None>(&SF.LeakedArgs)) {
    OS << "None"
       << " ";
  } else if (auto pval = std::get_if<std::vector<unsigned>>(&SF.LeakedArgs)) {
    for (auto Arg : *pval)
      OS << Arg << " ";
  } else {
    throw std::runtime_error("Something went wrong, unexpected type");
  }

  return OS << "]\n";
}

bool operator==(const TaintConfiguration::SinkFunction &Lhs,
                const TaintConfiguration::SinkFunction &Rhs) {
  return Lhs.Name == Rhs.Name && Lhs.LeakedArgs == Rhs.LeakedArgs;
}

bool TaintConfiguration::SinkFunction::isLeakedArg(unsigned ArgIdx) {
  if (auto pval = std::get_if<TaintConfiguration::All>(&LeakedArgs)) {
    return true;
  } else if (auto pval = std::get_if<TaintConfiguration::None>(&LeakedArgs)) {
    return false;
  } else if (auto pval = std::get_if<std::vector<unsigned>>(&LeakedArgs)) {
    return find(pval->begin(), pval->end(), ArgIdx) != pval->end();
  } else {
    throw std::runtime_error("Something went wrong, unexpected type");
  }
}

TaintConfiguration::TaintConfiguration(const std::string &FilePath) {
  importSourceSinkFunctions(FilePath);
}

TaintConfiguration::TaintConfiguration(
    std::initializer_list<SourceFunction> SourceFunctions,
    std::initializer_list<SinkFunction> SinkFunctions) {
      for (auto elem : SourceFunctions){
        Sources.insert(make_pair(elem.Name, elem));
      }
      for (auto elem : SinkFunctions){
        Sinks.insert(make_pair(elem.Name, elem));
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
                                                Fname, ReturnBool, ArgVec)));
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

void TaintConfiguration::addSource(TaintConfiguration::SourceFunction src){
  Sources.insert(make_pair(src.Name, src));
}

void TaintConfiguration::addSink(TaintConfiguration::SinkFunction snk){
  Sinks.insert(make_pair(snk.Name, snk));
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