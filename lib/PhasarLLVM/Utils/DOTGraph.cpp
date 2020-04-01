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

#include "boost/filesystem.hpp"

#include "nlohmann/json.hpp"

#include "phasar/Config/Configuration.h"
#include "phasar/PhasarLLVM/Utils/DOTGraph.h"

namespace psr {

DOTNode::DOTNode(std::string FName, std::string L, std::string SId,
                 unsigned FId, bool IsStmt, bool IsV)
    : funcName(FName), label(L), stmtId(SId), factId(FId), isVisible(IsV) {
  if (IsStmt) {
    id = funcName + '_' + stmtId;
  } else {
    id = funcName + '_' + std::to_string(factId) + '_' + stmtId;
  }
}

std::string DOTNode::str(std::string Indent) const {
  std::string str = Indent + id + " [label=\"" + label;
  if (factId) {
    str += " | SID: " + stmtId;
  }
  str += "\"";
  if (!isVisible) {
    str += ", style=invis";
  }
  return str + ']';
}

DOTEdge::DOTEdge(DOTNode Src, DOTNode Tar, bool IsV, std::string Efl,
                 std::string Vl)
    : source(Src), target(Tar), isVisible(IsV), edgeFnLabel(Efl),
      valueLabel(Vl) {}

std::string DOTEdge::str(std::string Indent) const {
  std::string str = Indent + source.id + " -> " + target.id;
  if (isVisible) {
    if (!edgeFnLabel.empty() && !valueLabel.empty()) {
      str += " [headlabel=\"\\r" + edgeFnLabel + "\", taillabel=\"" +
             valueLabel + "\"]";
    } else if (!edgeFnLabel.empty()) {
      str += " [headlabel=\"\\r" + edgeFnLabel + "\"]";
    } else if (!valueLabel.empty()) {
      str += " [taillabel=\"" + valueLabel + "\"]";
    }
  } else if (!isVisible) {
    str += " [style=invis]";
  }
  return str;
}

std::string DOTFactSubGraph::str(std::string Indent) const {
  std::string innerIndent = Indent + "  ";
  std::string str = Indent + "subgraph cluster_" + id + " {\n" + innerIndent +
                    "style=invis\n" + innerIndent + "label=\"" + label +
                    "\"\n\n" + innerIndent + "// Fact nodes in the ESG\n" +
                    innerIndent + DOTConfig::FactNodeAttr() + '\n';
  // Print fact nodes
  for (auto n : nodes) {
    str += n.second.str(innerIndent) + '\n';
  }
  // Print id edges
  str += '\n' + innerIndent + "// Identity edges for this fact\n" +
         innerIndent + DOTConfig::FactIDEdgeAttr() + '\n';
  for (DOTEdge e : edges) {
    str += e.str(innerIndent) + '\n';
  }
  return str + Indent + '}';
}

std::string DOTFunctionSubGraph::str(std::string Indent) const {
  std::string innerIndent = Indent + "  ";
  std::string str = Indent + "subgraph cluster_" + id + " {\n" + innerIndent +
                    "label=\"" + id + "\"";
  // Print control flow nodes
  str += "\n\n" + innerIndent + "// Control flow nodes\n" + innerIndent +
         DOTConfig::CFNodeAttr() + '\n';
  for (DOTNode stmt : stmts) {
    str += stmt.str(innerIndent) + '\n';
  }

  // Print fact subgraphs
  str += '\n' + innerIndent + "// Fact subgraphs\n";
  for (auto factSG : facts) {
    str += factSG.second.str(innerIndent) + "\n\n";
  }

  // Print lambda subgraph
  str += generateLambdaSG(innerIndent);

  // Print intra control flow edges
  str += "\n\n" + innerIndent + "// Intra-procedural control flow edges\n" +
         innerIndent + DOTConfig::CFIntraEdgeAttr() + '\n';
  for (DOTEdge e : intraCFEdges) {
    str += e.str(innerIndent) + '\n';
  }

  // Print intra cross fact edges
  str += '\n' + innerIndent + "// Intra-procedural cross fact edges\n" +
         innerIndent + DOTConfig::FactCrossEdgeAttr() + '\n';
  for (DOTEdge e : crossFactEdges) {
    str += e.str(innerIndent) + '\n';
  }
  return str + Indent + '}';
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

std::string DOTFunctionSubGraph::generateLambdaSG(std::string Indent) const {
  std::string innerIndent = Indent + "  ";
  std::string str = Indent + "// Auto-generated lambda nodes and edges\n" +
                    Indent + "subgraph cluster_" + id + "_lambda {\n" +
                    innerIndent + "style=invis\n" + innerIndent +
                    "label=\"Λ\"\n" + innerIndent +
                    DOTConfig::LambdaNodeAttr() + '\n';
  // Print lambda nodes
  for (DOTNode stmt : stmts) {
    str += innerIndent + id + "_0_" + stmt.stmtId +
           " [label=\"Λ|SID: " + stmt.stmtId + "\"]\n";
  }
  // Print lambda edges
  str += '\n' + innerIndent + DOTConfig::LambdaIDEdgeAttr() + '\n';
  for (DOTEdge e : intraCFEdges) {
    str += innerIndent + id + "_0_" + e.source.stmtId + " -> " + id + "_0_" +
           e.target.stmtId;
    if (e.isVisible) {
      str += " [headlabel=\"\\rAllBottom\", taillabel=\"BOT\"]\n";
    } else {
      str += " [style=invis]\n";
    }
  }
  return str + Indent + '}';
}

void DOTFunctionSubGraph::createLayoutCFNodes() {
  auto last = stmts.empty() ? stmts.end() : std::prev(stmts.end());
  for (auto firstIt = stmts.begin(); firstIt != last; ++firstIt) {
    auto secondIt = std::next(firstIt);
    DOTNode n1 = *firstIt;
    DOTNode n2 = *secondIt;
    intraCFEdges.emplace(n1, n2, false);
  }
}

void DOTFunctionSubGraph::createLayoutFactNodes() {
  for (auto &[key, factSG] : facts) {
    for (auto stmt : stmts) {
      if (factSG.nodes.find(stmt.stmtId) == factSG.nodes.end()) {
        DOTNode factNode(stmt.funcName, factSG.label, stmt.stmtId,
                         factSG.factId, false, false);
        factSG.nodes[stmt.stmtId] = factNode;
      }
    }
  }
}

void DOTFunctionSubGraph::createLayoutFactEdges() {
  for (auto &[key, factSG] : facts) {
    for (auto iCFE : intraCFEdges) {
      DOTNode d1 = {iCFE.source.funcName, factSG.label, iCFE.source.stmtId,
                    factSG.factId, false};
      DOTNode d2 = {iCFE.target.funcName, factSG.label, iCFE.target.stmtId,
                    factSG.factId, false};
      factSG.edges.emplace(d1, d2, false);
    }
  }
}

bool operator<(const DOTNode &Lhs, const DOTNode &Rhs) {
  stringIDLess strLess;
  // comparing control flow nodes
  if (Lhs.factId == 0 && Rhs.factId == 0) {
    return strLess(Lhs.stmtId, Rhs.stmtId);
  } else { // comparing fact nodes
    if (Lhs.factId == Rhs.factId) {
      return strLess(Lhs.stmtId, Rhs.stmtId);
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

void DOTConfig::importDOTConfig() {
  boost::filesystem::path FilePath(PhasarConfig::PhasarDirectory());
  FilePath /= boost::filesystem::path("config/DOTGraphConfig.json");
  if (boost::filesystem::exists(FilePath) &&
      !boost::filesystem::is_directory(FilePath)) {
    std::ifstream ifs(FilePath.string());
    if (ifs.is_open()) {
      std::stringstream iss;
      iss << ifs.rdbuf();
      ifs.close();
      nlohmann::json jDOTConfig;
      iss >> jDOTConfig;
      for (auto &el : jDOTConfig.items()) {
        std::stringstream attr_str;
        if (el.key().find("Node") != std::string::npos) {
          attr_str << "node [";
        } else {
          attr_str << "edge [";
        }
        for (nlohmann::json::iterator it = el.value().begin();
             it != el.value().end(); ++it) {
          // using it.value() directly with the << operator adds unnecessary
          // quotes
          std::string val = it.value();
          attr_str << it.key() << "=" << val;
          if (std::next(it) != el.value().end()) {
            attr_str << ", ";
          }
        }
        attr_str << ']';
        if (el.key() == "CFNode") {
          DOTConfig::CFNode = attr_str.str();
        } else if (el.key() == "CFIntraEdge") {
          DOTConfig::CFIntraEdge = attr_str.str();
        } else if (el.key() == "CFInterEdge") {
          DOTConfig::CFInterEdge = attr_str.str();
        } else if (el.key() == "FactNode") {
          DOTConfig::FactNode = attr_str.str();
        } else if (el.key() == "FactIDEdge") {
          DOTConfig::FactIDEdge = attr_str.str();
        } else if (el.key() == "FactCrossEdge") {
          DOTConfig::FactCrossEdge = attr_str.str();
        } else if (el.key() == "FactInterEdge") {
          DOTConfig::FactInterEdge = attr_str.str();
        } else if (el.key() == "LambdaNode") {
          DOTConfig::LambdaNode = attr_str.str();
        } else if (el.key() == "LambdaIDEdge") {
          DOTConfig::LambdaIDEdge = attr_str.str();
        } else if (el.key() == "LambdaInterEdge") {
          DOTConfig::LambdaInterEdge = attr_str.str();
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
