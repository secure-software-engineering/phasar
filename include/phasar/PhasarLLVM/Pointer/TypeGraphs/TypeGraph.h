/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * TypeGraph.h
 *
 *  Created on: 28.06.2018
 *      Author: nicolas bellec
 */

#ifndef PHASAR_PHASARLLVM_POINTER_TYPEGRAPHS_TYPEGRAPH_H_
#define PHASAR_PHASARLLVM_POINTER_TYPEGRAPHS_TYPEGRAPH_H_

#include <map>
#include <set>
#include <string>

namespace llvm {
class StructType;
}

namespace psr {
template <class ConcreteTypeGraph> class TypeGraph {
public:
  TypeGraph() = default;

  virtual ~TypeGraph() = default;
  // TypeGraph(const TypeGraph &copy) = delete;
  // TypeGraph& operator=(const TypeGraph &copy) = delete;
  // TypeGraph(TypeGraph &&move) = delete;
  // TypeGraph& operator=(TypeGraph &&move) = delete;

  /* Add a link if not already in the graph
   * Return true if the node has been added, false otherwise
   */
  virtual bool addLink(const llvm::StructType *from,
                       const llvm::StructType *to) = 0;
  virtual void printAsDot(const std::string &path = "typegraph.dot") const = 0;
  virtual std::set<const llvm::StructType *>
  getTypes(const llvm::StructType *struct_type) = 0;
};
} // namespace psr

#endif
