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

#ifndef PHASAR_PHASARLLVM_UTILS_DOTGRAPH_H_
#define PHASAR_PHASARLLVM_UTILS_DOTGRAPH_H_

#include <iosfwd>
#include <map>
#include <set>
#include <string>

#include "phasar/Config/Configuration.h"
#include "phasar/Utils/Utilities.h"

namespace psr {

class DOTConfig {
public:
  static const std::string CFNodeAttr() { return CFNode; }
  static const std::string CFIntraEdgeAttr() { return CFIntraEdge; }
  static const std::string CFInterEdgeAttr() { return CFInterEdge; }

  static const std::string FactNodeAttr() { return FactNode; }
  static const std::string FactIDEdgeAttr() { return FactIDEdge; }
  static const std::string FactCrossEdgeAttr() { return FactCrossEdge; }
  static const std::string FactInterEdgeAttr() { return FactInterEdge; }

  static const std::string LambdaNodeAttr() { return LambdaNode; }
  static const std::string LambdaIDEdgeAttr() { return LambdaIDEdge; }
  static const std::string LambdaInterEdgeAttr() { return LambdaInterEdge; }

  static void
  importDOTConfig(std::string ConfigPath = PhasarConfig::PhasarDirectory());

  static DOTConfig &getDOTConfig();
  ~DOTConfig() = default;
  DOTConfig(const DOTConfig &) = delete;
  DOTConfig(DOTConfig &&) = delete;

private:
  DOTConfig() = default;
  inline static const std::string fontSize = "fontsize=11";
  inline static const std::string arrowSize = "arrowsize=0.7";

  inline static std::string CFNode = "node [style=filled, shape=record]";
  inline static std::string CFIntraEdge = "edge []";
  inline static std::string CFInterEdge = "edge [weight=0.1]";
  inline static std::string FactNode = "node [style=rounded]";
  inline static std::string FactIDEdge =
      "edge [style=dotted, arrowhead=normal, " + fontSize + ", " + arrowSize +
      ']';
  inline static std::string FactCrossEdge =
      "edge [style=dotted, arrowhead=normal, " + fontSize + ", " + arrowSize +
      ']';
  inline static std::string FactInterEdge =
      "edge [weight=0.1, style=dashed, " + fontSize + ", " + arrowSize + ']';
  inline static std::string LambdaNode = "node [style=rounded]";
  inline static std::string LambdaIDEdge =
      "edge [style=dotted, arrowhead=normal, " + fontSize + ", " + arrowSize +
      ']';
  inline static std::string LambdaInterEdge =
      "edge [weight=0.1, style=dashed, " + fontSize + ", " + arrowSize + ']';
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
  std::string id;
  std::string funcName;
  std::string label;
  std::string stmtId;
  unsigned factId;
  bool isVisible = true;

  DOTNode() = default;
  DOTNode(std::string fName, std::string l, std::string sId, unsigned fId = 0,
          bool isStmt = true, bool isv = true);
  std::string str(const std::string &indent = "") const;
};

bool operator<(const DOTNode &lhs, const DOTNode &rhs);
bool operator==(const DOTNode &lhs, const DOTNode &rhs);
std::ostream &operator<<(std::ostream &os, const DOTNode &node);

struct DOTEdge {
  DOTNode source;
  DOTNode target;
  bool isVisible;
  std::string edgeFnLabel;
  std::string valueLabel;

  DOTEdge(DOTNode src, DOTNode tar, bool isv = true, std::string efl = "",
          std::string vl = "");
  std::string str(const std::string &indent = "") const;
};

bool operator<(const DOTEdge &lhs, const DOTEdge &rhs);
std::ostream &operator<<(std::ostream &os, const DOTEdge &edge);

struct DOTFactSubGraph {
  // fact subgraph id = <func-name>_<fact-id>
  std::string id;
  unsigned factId;
  std::string label;
  // stmt-id -> fact-node
  std::map<std::string, DOTNode, stringIDLess> nodes;
  std::set<DOTEdge> edges;

  std::string str(const std::string &indent = "") const;
};

std::ostream &operator<<(std::ostream &os, const DOTFactSubGraph &factSG);

struct DOTFunctionSubGraph {
  // function subgraph id = <func-name>
  std::string id;
  std::set<DOTNode> stmts;
  // fact-id -> fact-subgraph
  std::map<unsigned, DOTFactSubGraph> facts;
  std::set<DOTEdge> intraCFEdges;
  /// d1 -> d2 where d1 != d2
  std::set<DOTEdge> crossFactEdges;

  std::string str(const std::string &indent = "") const;
  DOTFactSubGraph *getOrCreateFactSG(unsigned factID, std::string &label);
  // TODO: pass the actual lambda EF name and value as parameter from DOTGraph
  std::string generateLambdaSG(const std::string &indent = "") const;
  void createLayoutCFNodes();
  void createLayoutFactNodes();
  void createLayoutFactEdges();
};

std::ostream &operator<<(std::ostream &os,
                         const DOTFunctionSubGraph &functionSG);

template <typename D> struct DOTGraph {
  std::string label;
  std::map<std::string, DOTFunctionSubGraph> functions;
  std::set<DOTEdge> interCFEdges;
  std::set<DOTEdge> interLambdaEdges;
  std::set<DOTEdge> interFactEdges;

  DOTGraph() = default;

  unsigned getFactID(D fact) {
    unsigned id = 0;
    if (DtoFactId.count(fact)) {
      id = DtoFactId[fact];
    } else {
      id = factIDCount;
      DtoFactId[fact] = factIDCount++;
    }
    return id;
  }

  bool containsFactSG(std::string fName, unsigned factID) {
    if (functions.count(fName)) {
      if (functions[fName].facts.count(factID)) {
        return true;
      }
    }
    return false;
  }

  std::string str() const {
    std::string indent = "  ";
    std::string str = "digraph {\n" + indent + "label=\"" + label + "\"\n";
    // Print function subgraphs
    str += '\n' + indent + "// Function sub graphs\n";
    for (auto fsg : functions) {
      fsg.second.createLayoutCFNodes();
      fsg.second.createLayoutFactNodes();
      fsg.second.createLayoutFactEdges();
      str += fsg.second.str(indent) + "\n\n";
    }

    // Print inter control flow edges
    str += indent + "// Inter-procedural control flow edges\n" + indent +
           DOTConfig::CFInterEdgeAttr() + '\n';
    for (DOTEdge e : interCFEdges) {
      str += e.str(indent) + '\n';
    }

    // Print inter lambda edges
    str += '\n' + indent + "// Inter-procedural lambda edges\n" + indent +
           DOTConfig::LambdaInterEdgeAttr() + '\n';
    for (DOTEdge e : interLambdaEdges) {
      str += e.str(indent) + '\n';
    }

    // Print inter fact edges
    str += '\n' + indent + "// Inter-procedural fact edges\n" + indent +
           DOTConfig::FactInterEdgeAttr() + '\n';
    for (DOTEdge e : interFactEdges) {
      str += e.str(indent) + '\n';
    }
    return str + '}';
  }

  friend std::ostream &operator<<(std::ostream &os, const DOTGraph<D> &graph) {
    return os << graph.str();
  }

private:
  // We introduce a fact-ID for data-flow facts D since only statements N have
  // an ID
  unsigned factIDCount = 1;
  std::map<D, unsigned> DtoFactId;
};

} // namespace psr

#endif
