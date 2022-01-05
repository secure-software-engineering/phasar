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
    : FuncName(std::move(FName)), Label(std::move(L)), StmtId(std::move(SId)),
      FactId(FId), IsVisible(IsV) {
  if (IsStmt) {
    Id = FuncName + '_' + StmtId;
  } else {
    Id = FuncName + '_' + std::to_string(FactId) + '_' + StmtId;
  }
}

std::string DOTNode::str(const std::string &Indent) const {
  std::string Str = Indent + Id + " [label=\"" + Label;
  if (FactId) {
    Str += " | SID: " + StmtId;
  }
  Str += "\"";
  if (!IsVisible) {
    Str += ", style=invis";
  }
  return Str + ']';
}

DOTEdge::DOTEdge(DOTNode Src, DOTNode Tar, bool IsV, std::string Efl,
                 std::string Vl)
    : Source(std::move(Src)), Target(std::move(Tar)), IsVisible(IsV),
      EdgeFnLabel(std::move(Efl)), ValueLabel(std::move(Vl)) {}

std::string DOTEdge::str(const std::string &Indent) const {
  std::string Str = Indent + Source.Id + " -> " + Target.Id;
  if (IsVisible) {
    if (!EdgeFnLabel.empty() && !ValueLabel.empty()) {
      Str += " [headlabel=\"\\r" + EdgeFnLabel + "\", taillabel=\"" +
             ValueLabel + "\"]";
    } else if (!EdgeFnLabel.empty()) {
      Str += " [headlabel=\"\\r" + EdgeFnLabel + "\"]";
    } else if (!ValueLabel.empty()) {
      Str += " [taillabel=\"" + ValueLabel + "\"]";
    }
  } else if (!IsVisible) {
    Str += " [style=invis]";
  }
  return Str;
}

std::string DOTFactSubGraph::str(const std::string &Indent) const {
  std::string InnerIndent = Indent + "  ";
  std::string Str = Indent + "subgraph cluster_" + Id + " {\n" + InnerIndent +
                    "style=invis\n" + InnerIndent + "label=\"" + Label +
                    "\"\n\n" + InnerIndent + "// Fact nodes in the ESG\n" +
                    InnerIndent + DOTConfig::FactNodeAttr() + '\n';
  // Print fact nodes
  for (const auto &N : Nodes) {
    Str += N.second.str(InnerIndent) + '\n';
  }
  // Print id edges
  Str += '\n' + InnerIndent + "// Identity edges for this fact\n" +
         InnerIndent + DOTConfig::FactIDEdgeAttr() + '\n';
  for (const DOTEdge &E : Edges) {
    Str += E.str(InnerIndent) + '\n';
  }
  return Str + Indent + '}';
}

std::string DOTFunctionSubGraph::str(const std::string &Indent) const {
  std::string InnerIndent = Indent + "  ";
  std::string Str = Indent + "subgraph cluster_" + Id + " {\n" + InnerIndent +
                    "label=\"" + Id + "\"";
  // Print control flow nodes
  Str += "\n\n" + InnerIndent + "// Control flow nodes\n" + InnerIndent +
         DOTConfig::CFNodeAttr() + '\n';
  for (const DOTNode &Stmt : Stmts) {
    Str += Stmt.str(InnerIndent) + '\n';
  }

  // Print fact subgraphs
  Str += '\n' + InnerIndent + "// Fact subgraphs\n";
  for (const auto &FactSG : Facts) {
    Str += FactSG.second.str(InnerIndent) + "\n\n";
  }

  // Print lambda subgraph
  Str += generateLambdaSG(InnerIndent);

  // Print intra control flow edges
  Str += "\n\n" + InnerIndent + "// Intra-procedural control flow edges\n" +
         InnerIndent + DOTConfig::CFIntraEdgeAttr() + '\n';
  for (const DOTEdge &E : IntraCFEdges) {
    Str += E.str(InnerIndent) + '\n';
  }

  // Print intra cross fact edges
  Str += '\n' + InnerIndent + "// Intra-procedural cross fact edges\n" +
         InnerIndent + DOTConfig::FactCrossEdgeAttr() + '\n';
  for (const DOTEdge &E : CrossFactEdges) {
    Str += E.str(InnerIndent) + '\n';
  }
  return Str + Indent + '}';
}

DOTFactSubGraph *DOTFunctionSubGraph::getOrCreateFactSG(unsigned FactID,
                                                        std::string &Label) {
  DOTFactSubGraph *FuncSG = &Facts[FactID];
  if (FuncSG->Id.empty()) {
    FuncSG->Id = Id + '_' + std::to_string(FactID);
    FuncSG->FactId = FactID;
    FuncSG->Label = Label;
  }
  return FuncSG;
}

std::string
DOTFunctionSubGraph::generateLambdaSG(const std::string &Indent) const {
  std::string InnerIndent = Indent + "  ";
  std::string Str = Indent + "// Auto-generated lambda nodes and edges\n" +
                    Indent + "subgraph cluster_" + Id + "_lambda {\n" +
                    InnerIndent + "style=invis\n" + InnerIndent +
                    "label=\"Λ\"\n" + InnerIndent +
                    DOTConfig::LambdaNodeAttr() + '\n';
  // Print lambda nodes
  for (const DOTNode &Stmt : Stmts) {
    Str += InnerIndent + Id + "_0_" + Stmt.StmtId +
           " [label=\"Λ|SID: " + Stmt.StmtId + "\"]\n";
  }
  // Print lambda edges
  Str += '\n' + InnerIndent + DOTConfig::LambdaIDEdgeAttr() + '\n';
  for (const DOTEdge &E : IntraCFEdges) {
    Str += InnerIndent + Id + "_0_" + E.Source.StmtId + " -> " + Id + "_0_" +
           E.Target.StmtId;
    if (E.IsVisible) {
      Str += " [headlabel=\"\\rAllBottom\", taillabel=\"BOT\"]\n";
    } else {
      Str += " [style=invis]\n";
    }
  }
  return Str + Indent + '}';
}

void DOTFunctionSubGraph::createLayoutCFNodes() {
  auto Last = Stmts.empty() ? Stmts.end() : std::prev(Stmts.end());
  for (auto FirstIt = Stmts.begin(); FirstIt != Last; ++FirstIt) {
    auto SecondIt = std::next(FirstIt);
    DOTNode N1 = *FirstIt;
    DOTNode N2 = *SecondIt;
    IntraCFEdges.emplace(N1, N2, false);
  }
}

void DOTFunctionSubGraph::createLayoutFactNodes() {
  for (auto &[Key, FactSG] : Facts) {
    for (const auto &Stmt : Stmts) {
      if (FactSG.Nodes.find(Stmt.StmtId) == FactSG.Nodes.end()) {
        DOTNode FactNode(Stmt.FuncName, FactSG.Label, Stmt.StmtId,
                         FactSG.FactId, false, false);
        FactSG.Nodes[Stmt.StmtId] = FactNode;
      }
    }
  }
}

void DOTFunctionSubGraph::createLayoutFactEdges() {
  for (auto &[Key, FactSG] : Facts) {
    for (const auto &ICFE : IntraCFEdges) {
      DOTNode D1 = {ICFE.Source.FuncName, FactSG.Label, ICFE.Source.StmtId,
                    FactSG.FactId, false};
      DOTNode D2 = {ICFE.Target.FuncName, FactSG.Label, ICFE.Target.StmtId,
                    FactSG.FactId, false};
      FactSG.Edges.emplace(D1, D2, false);
    }
  }
}

bool operator<(const DOTNode &Lhs, const DOTNode &Rhs) {
  StringIDLess StrLess;
  // comparing control flow nodes
  if (Lhs.FactId == 0 && Rhs.FactId == 0) {
    return StrLess(Lhs.StmtId, Rhs.StmtId);
  } // comparing fact nodes
  if (Lhs.FactId == Rhs.FactId) {
    return StrLess(Lhs.StmtId, Rhs.StmtId);
  }
  return Lhs.FactId < Rhs.FactId;
}

bool operator==(const DOTNode &Lhs, const DOTNode &Rhs) {
  return !(Lhs < Rhs) && !(Rhs < Lhs);
}

std::ostream &operator<<(std::ostream &OS, const DOTNode &Node) {
  return OS << Node.str();
}

bool operator<(const DOTEdge &Lhs, const DOTEdge &Rhs) {
  if (Lhs.Source == Rhs.Source) {
    return Lhs.Target < Rhs.Target;
  }
  return Lhs.Source < Rhs.Source;
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
