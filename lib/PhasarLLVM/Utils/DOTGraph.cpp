/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * DOTGraph.cpp
 *
 *  Created on: 31.08.2019
 *      Author: rleer
 */

#include <algorithm>
#include <iterator>
#include <ostream>
#include <utility>

#include "boost/filesystem.hpp"

#include "nlohmann/json.hpp"

#include "phasar/PhasarLLVM/Utils/DOTGraph.h"

namespace psr {

DOTNode::DOTNode(std::string FName, std::string L, std::string SId,
                 unsigned FId, bool IsStmt, bool IsV)
    : funcName(std::move(FName)), label(std::move(L)), stmtId(std::move(SId)),
      factId(FId), isVisible(IsV) {
  if (IsStmt) {
    id = funcName + '_' + stmtId;
  } else {
    id = funcName + '_' + std::to_string(factId) + '_' + stmtId;
  }
}

std::string DOTNode::str(const std::string &Indent) const {
  std::string Str = Indent + id + " [label=\"" + label;
  if (factId) {
    Str += " | SID: " + stmtId;
  }
  Str += "\"";
  if (!isVisible) {
    Str += ", style=invis";
  }
  return Str + ']';
}

DOTEdge::DOTEdge(DOTNode Src, DOTNode Tar, bool IsV, std::string Efl,
                 std::string Vl)
    : source(std::move(Src)), target(std::move(Tar)), isVisible(IsV),
      edgeFnLabel(std::move(Efl)), valueLabel(std::move(Vl)) {}

std::string DOTEdge::str(const std::string &Indent) const {
  std::string Str = Indent + source.id + " -> " + target.id;
  if (isVisible) {
    if (!edgeFnLabel.empty() && !valueLabel.empty()) {
      Str += " [headlabel=\"\\r" + edgeFnLabel + "\", taillabel=\"" +
             valueLabel + "\"]";
    } else if (!edgeFnLabel.empty()) {
      Str += " [headlabel=\"\\r" + edgeFnLabel + "\"]";
    } else if (!valueLabel.empty()) {
      Str += " [taillabel=\"" + valueLabel + "\"]";
    }
  } else if (!isVisible) {
    Str += " [style=invis]";
  }
  return Str;
}

std::string DOTFactSubGraph::str(const std::string &Indent) const {
  std::string InnerIndent = Indent + "  ";
  std::string Str = Indent + "subgraph cluster_" + id + " {\n" + InnerIndent +
                    "style=invis\n" + InnerIndent + "label=\"" + label +
                    "\"\n\n" + InnerIndent + "// Fact nodes in the ESG\n" +
                    InnerIndent + DOTConfig::FactNodeAttr() + '\n';
  // Print fact nodes
  for (const auto &N : nodes) {
    Str += N.second.str(InnerIndent) + '\n';
  }
  // Print id edges
  Str += '\n' + InnerIndent + "// Identity edges for this fact\n" +
         InnerIndent + DOTConfig::FactIDEdgeAttr() + '\n';
  for (const DOTEdge &E : edges) {
    Str += E.str(InnerIndent) + '\n';
  }
  return Str + Indent + '}';
}

std::string DOTFunctionSubGraph::str(const std::string &Indent) const {
  std::string InnerIndent = Indent + "  ";
  std::string Str = Indent + "subgraph cluster_" + id + " {\n" + InnerIndent +
                    "label=\"" + id + "\"";
  // Print control flow nodes
  Str += "\n\n" + InnerIndent + "// Control flow nodes\n" + InnerIndent +
         DOTConfig::CFNodeAttr() + '\n';
  for (const DOTNode &Stmt : stmts) {
    Str += Stmt.str(InnerIndent) + '\n';
  }

  // Print fact subgraphs
  Str += '\n' + InnerIndent + "// Fact subgraphs\n";
  for (const auto &FactSG : facts) {
    Str += FactSG.second.str(InnerIndent) + "\n\n";
  }

  // Print lambda subgraph
  Str += generateLambdaSG(InnerIndent);

  // Print intra control flow edges
  Str += "\n\n" + InnerIndent + "// Intra-procedural control flow edges\n" +
         InnerIndent + DOTConfig::CFIntraEdgeAttr() + '\n';
  for (const DOTEdge &E : intraCFEdges) {
    Str += E.str(InnerIndent) + '\n';
  }

  // Print intra cross fact edges
  Str += '\n' + InnerIndent + "// Intra-procedural cross fact edges\n" +
         InnerIndent + DOTConfig::FactCrossEdgeAttr() + '\n';
  for (const DOTEdge &E : crossFactEdges) {
    Str += E.str(InnerIndent) + '\n';
  }
  return Str + Indent + '}';
}

DOTFactSubGraph *DOTFunctionSubGraph::getOrCreateFactSG(unsigned FactID,
                                                        std::string &Label) {
  DOTFactSubGraph *FuncSG = &facts[FactID];
  if (FuncSG->id.empty()) {
    FuncSG->id = id + '_' + std::to_string(FactID);
    FuncSG->factId = FactID;
    FuncSG->label = Label;
  }
  return FuncSG;
}

std::string
DOTFunctionSubGraph::generateLambdaSG(const std::string &Indent) const {
  std::string InnerIndent = Indent + "  ";
  std::string Str = Indent + "// Auto-generated lambda nodes and edges\n" +
                    Indent + "subgraph cluster_" + id + "_lambda {\n" +
                    InnerIndent + "style=invis\n" + InnerIndent +
                    "label=\"Λ\"\n" + InnerIndent +
                    DOTConfig::LambdaNodeAttr() + '\n';
  // Print lambda nodes
  for (const DOTNode &Stmt : stmts) {
    Str += InnerIndent + id + "_0_" + Stmt.stmtId +
           " [label=\"Λ|SID: " + Stmt.stmtId + "\"]\n";
  }
  // Print lambda edges
  Str += '\n' + InnerIndent + DOTConfig::LambdaIDEdgeAttr() + '\n';
  for (const DOTEdge &E : intraCFEdges) {
    Str += InnerIndent + id + "_0_" + E.source.stmtId + " -> " + id + "_0_" +
           E.target.stmtId;
    if (E.isVisible) {
      Str += " [headlabel=\"\\rAllBottom\", taillabel=\"BOT\"]\n";
    } else {
      Str += " [style=invis]\n";
    }
  }
  return Str + Indent + '}';
}

void DOTFunctionSubGraph::createLayoutCFNodes() {
  auto Last = stmts.empty() ? stmts.end() : std::prev(stmts.end());
  for (auto FirstIt = stmts.begin(); FirstIt != Last; ++FirstIt) {
    auto SecondIt = std::next(FirstIt);
    DOTNode N1 = *FirstIt;
    DOTNode N2 = *SecondIt;
    intraCFEdges.emplace(N1, N2, false);
  }
}

void DOTFunctionSubGraph::createLayoutFactNodes() {
  for (auto &[Key, FactSG] : facts) {
    for (const auto &Stmt : stmts) {
      if (FactSG.nodes.find(Stmt.stmtId) == FactSG.nodes.end()) {
        DOTNode FactNode(Stmt.funcName, FactSG.label, Stmt.stmtId,
                         FactSG.factId, false, false);
        FactSG.nodes[Stmt.stmtId] = FactNode;
      }
    }
  }
}

void DOTFunctionSubGraph::createLayoutFactEdges() {
  for (auto &[Key, FactSG] : facts) {
    for (const auto &ICFE : intraCFEdges) {
      DOTNode D1 = {ICFE.source.funcName, FactSG.label, ICFE.source.stmtId,
                    FactSG.factId, false};
      DOTNode D2 = {ICFE.target.funcName, FactSG.label, ICFE.target.stmtId,
                    FactSG.factId, false};
      FactSG.edges.emplace(D1, D2, false);
    }
  }
}

bool operator<(const DOTNode &Lhs, const DOTNode &Rhs) {
  stringIDLess StrLess;
  // comparing control flow nodes
  if (Lhs.factId == 0 && Rhs.factId == 0) {
    return StrLess(Lhs.stmtId, Rhs.stmtId);
  } else { // comparing fact nodes
    if (Lhs.factId == Rhs.factId) {
      return StrLess(Lhs.stmtId, Rhs.stmtId);
    } else {
      return Lhs.factId < Rhs.factId;
    }
  }
}

bool operator==(const DOTNode &Lhs, const DOTNode &Rhs) {
  return !(Lhs < Rhs) && !(Rhs < Lhs);
}

std::ostream &operator<<(std::ostream &OS, const DOTNode &Node) {
  return OS << Node.str();
}

bool operator<(const DOTEdge &Lhs, const DOTEdge &Rhs) {
  if (Lhs.source == Rhs.source) {
    return Lhs.target < Rhs.target;
  }
  return Lhs.source < Rhs.source;
}

std::ostream &operator<<(std::ostream &OS, const DOTEdge &Edge) {
  return OS << Edge.str();
}

std::ostream &operator<<(std::ostream &OS, const DOTFactSubGraph &FactSG) {
  return OS << FactSG.str();
}

std::ostream &operator<<(std::ostream &OS,
                         const DOTFunctionSubGraph &FunctionSG) {
  return OS << FunctionSG.str();
}

DOTConfig &DOTConfig::getDOTConfig() {
  static DOTConfig DC;
  return DC;
}

void DOTConfig::importDOTConfig(std::string ConfigPath) {
  boost::filesystem::path FilePath(ConfigPath);
  FilePath /= boost::filesystem::path("config/DOTGraphConfig.json");
  if (boost::filesystem::exists(FilePath) &&
      !boost::filesystem::is_directory(FilePath)) {
    std::ifstream Ifs(FilePath.string());
    if (Ifs.is_open()) {
      std::stringstream Iss;
      Iss << Ifs.rdbuf();
      Ifs.close();
      nlohmann::json JDOTConfig;
      Iss >> JDOTConfig;
      for (auto &El : JDOTConfig.items()) {
        std::stringstream AttrStr;
        if (El.key().find("Node") != std::string::npos) {
          AttrStr << "node [";
        } else {
          AttrStr << "edge [";
        }
        for (nlohmann::json::iterator It = El.value().begin();
             It != El.value().end(); ++It) {
          // using it.value() directly with the << operator adds unnecessary
          // quotes
          std::string Val = It.value();
          AttrStr << It.key() << "=" << Val;
          if (std::next(It) != El.value().end()) {
            AttrStr << ", ";
          }
        }
        AttrStr << ']';
        if (El.key() == "CFNode") {
          DOTConfig::CFNode = AttrStr.str();
        } else if (El.key() == "CFIntraEdge") {
          DOTConfig::CFIntraEdge = AttrStr.str();
        } else if (El.key() == "CFInterEdge") {
          DOTConfig::CFInterEdge = AttrStr.str();
        } else if (El.key() == "FactNode") {
          DOTConfig::FactNode = AttrStr.str();
        } else if (El.key() == "FactIDEdge") {
          DOTConfig::FactIDEdge = AttrStr.str();
        } else if (El.key() == "FactCrossEdge") {
          DOTConfig::FactCrossEdge = AttrStr.str();
        } else if (El.key() == "FactInterEdge") {
          DOTConfig::FactInterEdge = AttrStr.str();
        } else if (El.key() == "LambdaNode") {
          DOTConfig::LambdaNode = AttrStr.str();
        } else if (El.key() == "LambdaIDEdge") {
          DOTConfig::LambdaIDEdge = AttrStr.str();
        } else if (El.key() == "LambdaInterEdge") {
          DOTConfig::LambdaInterEdge = AttrStr.str();
        }
      }
    } else {
      throw std::ios_base::failure("Could not open file");
    }
  } else {
    throw std::ios_base::failure(FilePath.string() + " is not a valid path");
  }
}

} // namespace psr
