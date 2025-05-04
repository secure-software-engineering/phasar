/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * DOTGraph.h
 *
 *  Created on: 31.08.2019
 *      Author: rleer
 */

#ifndef PHASAR_UTILS_DOTGRAPH_H
#define PHASAR_UTILS_DOTGRAPH_H

#include "phasar/Config/Configuration.h"
#include "phasar/Utils/Utilities.h"

#include <map>
#include <set>
#include <string>

namespace psr {

class DOTConfig {
public:
  static const std::string CFNodeAttr() { return CFNode; }           // NOLINT
  static const std::string CFIntraEdgeAttr() { return CFIntraEdge; } // NOLINT
  static const std::string CFInterEdgeAttr() { return CFInterEdge; } // NOLINT

  static const std::string FactNodeAttr() { return FactNode; }     // NOLINT
  static const std::string FactIDEdgeAttr() { return FactIDEdge; } // NOLINT
  static const std::string FactCrossEdgeAttr() {                   // NOLINT
    return FactCrossEdge;
  }
  static const std::string FactInterEdgeAttr() { // NOLINT
    return FactInterEdge;
  }

  static const std::string LambdaNodeAttr() { return LambdaNode; }     // NOLINT
  static const std::string LambdaIDEdgeAttr() { return LambdaIDEdge; } // NOLINT
  static const std::string LambdaInterEdgeAttr() {                     // NOLINT
    return LambdaInterEdge;
  }

  static void
  importDOTConfig(llvm::StringRef ConfigPath = PhasarConfig::PhasarDirectory());

  static DOTConfig &getDOTConfig();
  ~DOTConfig() = default;
  DOTConfig(const DOTConfig &) = delete;
  DOTConfig(DOTConfig &&) = delete;
  DOTConfig &operator=(const DOTConfig &) = delete;
  DOTConfig &operator=(const DOTConfig &&) = delete;

private:
  DOTConfig() = default;
  inline static const std::string FontSize = "fontsize=11";
  inline static const std::string ArrowSize = "arrowsize=0.7";

  inline static std::string CFNode = // NOLINT
      "node [style=filled, shape=record]";
  inline static std::string CFIntraEdge = "edge []";           // NOLINT
  inline static std::string CFInterEdge = "edge [weight=0.1]"; // NOLINT
  inline static std::string FactNode = "node [style=rounded]"; // NOLINT
  inline static std::string FactIDEdge =                       // NOLINT
      "edge [style=dotted, arrowhead=normal, " + FontSize + ", " + ArrowSize +
      ']';
  inline static std::string FactCrossEdge = // NOLINT
      "edge [style=dotted, arrowhead=normal, " + FontSize + ", " + ArrowSize +
      ']';
  inline static std::string FactInterEdge = // NOLINT
      "edge [weight=0.1, style=dashed, " + FontSize + ", " + ArrowSize + ']';
  inline static std::string LambdaNode = "node [style=rounded]"; // NOLINT
  inline static std::string LambdaIDEdge =                       // NOLINT
      "edge [style=dotted, arrowhead=normal, " + FontSize + ", " + ArrowSize +
      ']';
  inline static std::string LambdaInterEdge = // NOLINT
      "edge [weight=0.1, style=dashed, " + FontSize + ", " + ArrowSize + ']';
};

struct DOTNode {
  /* stmt id = <func-name>_<stmt-id>
   * e.g. foo_42 for the statement w/ id 42 in function foo
   *
   * fact id = <func-name>_<fact-id>_<stmt-id>
   * e.g. fact i in main valid at statement 3 : main_1_3
   * Note that in this example fact i is the very first valid fact
   * encountered so the fact-id is 1 (zero value has fact-id 0)
   */
  std::string Id;
  std::string FuncName;
  std::string Label;
  std::string StmtId;
  unsigned FactId = 0;
  bool IsVisible = true;

  DOTNode() = default;
  DOTNode(std::string FName, std::string L, std::string SId, unsigned FId = 0,
          bool IsStmt = true, bool Isv = true);
  [[nodiscard]] std::string str(const std::string &Indent = "") const;
};

bool operator<(const DOTNode &Lhs, const DOTNode &Rhs);
bool operator==(const DOTNode &Lhs, const DOTNode &Rhs);
llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const DOTNode &Node);

struct DOTEdge {
  DOTNode Source;
  DOTNode Target;
  bool IsVisible;
  std::string EdgeFnLabel;
  std::string ValueLabel;

  DOTEdge(DOTNode Src, DOTNode Tar, bool Isv = true, std::string Efl = "",
          std::string Vl = "");
  [[nodiscard]] std::string str(const std::string &Indent = "") const;
};

bool operator<(const DOTEdge &Lhs, const DOTEdge &Rhs);
llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const DOTEdge &Edge);

struct DOTFactSubGraph {
  // fact subgraph id = <func-name>_<fact-id>
  std::string Id;
  unsigned FactId = 0;
  std::string Label;
  // stmt-id -> fact-node
  std::map<std::string, DOTNode, StringIDLess> Nodes;
  std::set<DOTEdge> Edges;

  [[nodiscard]] std::string str(const std::string &Indent = "") const;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const DOTFactSubGraph &FactSG);

struct DOTFunctionSubGraph {
  // function subgraph id = <func-name>
  std::string Id;
  std::set<DOTNode> Stmts;
  // fact-id -> fact-subgraph
  std::map<unsigned, DOTFactSubGraph> Facts;
  std::set<DOTEdge> IntraCFEdges;
  /// d1 -> d2 where d1 != d2
  std::set<DOTEdge> CrossFactEdges;

  [[nodiscard]] std::string str(const std::string &Indent = "") const;
  DOTFactSubGraph *getOrCreateFactSG(unsigned FactID, std::string &Label);
  // TODO: pass the actual lambda EF name and value as parameter from DOTGraph
  [[nodiscard]] std::string
  generateLambdaSG(const std::string &Indent = "") const;
  void createLayoutCFNodes();
  void createLayoutFactNodes();
  void createLayoutFactEdges();
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                              const DOTFunctionSubGraph &FunctionSG);

template <typename D> struct DOTGraph {
  std::string Label;
  std::map<std::string, DOTFunctionSubGraph> Functions;
  std::set<DOTEdge> InterCFEdges;
  std::set<DOTEdge> InterLambdaEdges;
  std::set<DOTEdge> InterFactEdges;

  DOTGraph() = default;

  unsigned getFactID(D Fact) {
    unsigned Id = 0;
    if (DtoFactId.count(Fact)) {
      Id = DtoFactId[Fact];
    } else {
      Id = FactIDCount;
      DtoFactId[Fact] = FactIDCount++;
    }
    return Id;
  }

  bool containsFactSG(std::string &FName, unsigned FactId) {
    if (Functions.count(FName)) {
      if (Functions[FName].Facts.count(FactId)) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] std::string str() const {
    std::string Indent = "  ";
    std::string Str = "digraph {\n" + Indent + "label=\"" + Label + "\"\n";
    // Print function subgraphs
    Str += '\n' + Indent + "// Function sub graphs\n";
    for (auto Fsg : Functions) {
      Fsg.second.createLayoutCFNodes();
      Fsg.second.createLayoutFactNodes();
      Fsg.second.createLayoutFactEdges();
      Str += Fsg.second.str(Indent) + "\n\n";
    }

    // Print inter control flow edges
    Str += Indent + "// Inter-procedural control flow edges\n" + Indent +
           DOTConfig::CFInterEdgeAttr() + '\n';
    for (const DOTEdge &Edge : InterCFEdges) {
      Str += Edge.str(Indent) + '\n';
    }

    // Print inter lambda edges
    Str += '\n' + Indent + "// Inter-procedural lambda edges\n" + Indent +
           DOTConfig::LambdaInterEdgeAttr() + '\n';
    for (const DOTEdge &Edge : InterLambdaEdges) {
      Str += Edge.str(Indent) + '\n';
    }

    // Print inter fact edges
    Str += '\n' + Indent + "// Inter-procedural fact edges\n" + Indent +
           DOTConfig::FactInterEdgeAttr() + '\n';
    for (const DOTEdge &Edge : InterFactEdges) {
      Str += Edge.str(Indent) + '\n';
    }
    return Str + '}';
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const DOTGraph<D> &Graph) {
    return OS << Graph.str();
  }

private:
  // We introduce a fact-ID for data-flow facts D since only statements N have
  // an ID
  unsigned FactIDCount = 1;
  std::map<D, unsigned> DtoFactId;
};

} // namespace psr

#endif
