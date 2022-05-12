/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * JumpFunctions.h
 *
 *  Created on: 17.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_JUMPFUNCTIONS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_SOLVER_JUMPFUNCTIONS_H

#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <unordered_map>
#include <utility>

#include "llvm/ADT/SmallVector.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Table.h"

namespace psr {

// Forward declare the IDETabulationProblem as we require its toString
// functionality.
template <typename AnalysisDomainTy, typename Container>
class IDETabulationProblem;

template <typename AnalysisDomainTy, typename Container> class JumpFunctions {
public:
  using l_t = typename AnalysisDomainTy::l_t;
  using d_t = typename AnalysisDomainTy::d_t;
  using n_t = typename AnalysisDomainTy::n_t;

  using EdgeFunctionType = EdgeFunction<l_t>;
  using EdgeFunctionPtrType = std::shared_ptr<EdgeFunctionType>;

private:
  EdgeFunctionPtrType Alltop;
  const IDETabulationProblem<AnalysisDomainTy, Container> &Problem;

protected:
  // mapping from target node and value to a list of all source values and
  // associated functions where the list is implemented as a mapping from
  // the source value to the function we exclude empty default functions
  Table<n_t, d_t, llvm::SmallVector<std::pair<d_t, EdgeFunctionPtrType>, 1>>
      NonEmptyReverseLookup;
  // mapping from source value and target node to a list of all target values
  // and associated functions where the list is implemented as a mapping from
  // the source value to the function we exclude empty default functions
  Table<d_t, n_t, llvm::SmallVector<std::pair<d_t, EdgeFunctionPtrType>, 1>>
      NonEmptyForwardLookup;
  // a mapping from target node to a list of triples consisting of source value,
  // target value and associated function; the triple is implemented by a table
  // we exclude empty default functions
  std::unordered_map<n_t, Table<d_t, d_t, EdgeFunctionPtrType>>
      NonEmptyLookupByTargetNode;

public:
  JumpFunctions(
      EdgeFunctionPtrType Alltop,
      const IDETabulationProblem<AnalysisDomainTy, Container> &Problem)
      : Alltop(std::move(Alltop)), Problem(Problem) {}

  ~JumpFunctions() = default;

  JumpFunctions(const JumpFunctions &JFs) = default;
  JumpFunctions &operator=(const JumpFunctions &JFs) = default;
  JumpFunctions(JumpFunctions &&JFs) noexcept = default;
  JumpFunctions &operator=(JumpFunctions &&JFs) noexcept = default;

  /**
   * Records a jump function. The source statement is implicit.
   * @see PathEdge
   */
  void addFunction(d_t SourceVal, n_t Target, d_t TargetVal,
                   EdgeFunctionPtrType EdgeFunc) {
    PHASAR_LOG_LEVEL(DEBUG, "Start adding new jump function");
    PHASAR_LOG_LEVEL(DEBUG,
                     "Fact at source : " << Problem.DtoString(SourceVal));
    PHASAR_LOG_LEVEL(DEBUG,
                     "Fact at target : " << Problem.DtoString(TargetVal));
    PHASAR_LOG_LEVEL(DEBUG, "Destination    : " << Problem.NtoString(Target));
    PHASAR_LOG_LEVEL(DEBUG, "Edge Function  : " << EdgeFunc->str());
    // we do not store the default function (all-top)
    if (EdgeFunc->equal_to(Alltop)) {
      return;
    }

    auto &SourceValToFunc = NonEmptyReverseLookup.get(Target, TargetVal);
    if (auto Find = std::find_if(
            SourceValToFunc.begin(), SourceValToFunc.end(),
            [SourceVal](const std::pair<d_t, EdgeFunctionPtrType> &Entry) {
              return SourceVal == Entry.first;
            });
        Find != SourceValToFunc.end()) {
      // it is important that existing values in JumpFunctions
      // are overwritten
      Find->second = EdgeFunc;
    } else {
      SourceValToFunc.emplace_back(SourceVal, EdgeFunc);
    }

    auto &TargetValToFunc = NonEmptyForwardLookup.get(SourceVal, Target);
    if (auto Find = std::find_if(
            TargetValToFunc.begin(), TargetValToFunc.end(),
            [TargetVal](const std::pair<d_t, EdgeFunctionPtrType> &Entry) {
              return TargetVal == Entry.first;
            });
        Find != TargetValToFunc.end()) {
      // it is important that existing values in JumpFunctions
      // are overwritten
      Find->second = EdgeFunc;
    } else {
      TargetValToFunc.emplace_back(TargetVal, EdgeFunc);
    }

    // V Table::insert(R r, C c, V v) always overrides (see
    // comments above)
    NonEmptyLookupByTargetNode[Target].insert(SourceVal, TargetVal, EdgeFunc);
    PHASAR_LOG_LEVEL(DEBUG, "End adding new jump function");
  }

  /**
   * Returns, for a given target statement and value all associated
   * source values, and for each the associated edge function.
   * The return value is a mapping from source value to function.
   */
  std::optional<std::reference_wrapper<
      llvm::SmallVectorImpl<std::pair<d_t, EdgeFunctionPtrType>>>>
  reverseLookup(n_t Target, d_t TargetVal) {
    if (!NonEmptyReverseLookup.contains(Target, TargetVal)) {
      return std::nullopt;
    }
    return {NonEmptyReverseLookup.get(Target, TargetVal)};
  }

  /**
   * Returns, for a given source value and target statement all
   * associated target values, and for each the associated edge function.
   * The return value is a mapping from target value to function.
   */
  std::optional<std::reference_wrapper<
      llvm::SmallVectorImpl<std::pair<d_t, EdgeFunctionPtrType>>>>
  forwardLookup(d_t SourceVal, n_t Target) {
    if (!NonEmptyForwardLookup.contains(SourceVal, Target)) {
      return std::nullopt;
    }
    return {NonEmptyForwardLookup.get(SourceVal, Target)};
  }

  /**
   * Returns for a given target statement all jump function records with this
   * target.
   * The return value is a set of records of the form
   * (sourceVal,targetVal,edgeFunction).
   */
  Table<d_t, d_t, EdgeFunctionPtrType> &lookupByTarget(n_t Target) {
    return NonEmptyLookupByTargetNode[Target];
  }

  /**
   * Removes a jump function. The source statement is implicit.
   * @see PathEdge
   * @return True if the function has actually been removed. False if it was not
   * there anyway.
   */
  bool removeFunction(d_t SourceVal, n_t Target, d_t TargetVal) {
    auto &SourceValToFunc = NonEmptyReverseLookup.get(Target, TargetVal);
    if (auto Find = std::find_if(
            SourceValToFunc.begin(), SourceValToFunc.end(),
            [SourceVal](const std::pair<d_t, EdgeFunctionPtrType> &Entry) {
              return SourceVal == Entry.first;
            });
        Find != SourceValToFunc.end()) {
      SourceValToFunc.erase(Find);
    }
    auto &TargetValToFunc = NonEmptyForwardLookup.get(SourceVal, Target);
    if (auto Find = std::find_if(
            TargetValToFunc.begin(), TargetValToFunc.end(),
            [TargetVal](const std::pair<d_t, EdgeFunctionPtrType> &Entry) {
              return TargetVal == Entry.first;
            });
        Find != TargetValToFunc.end()) {
      TargetValToFunc.erase(Find);
    }
    return NonEmptyLookupByTargetNode.erase(Target);
  }

  /**
   * Removes all jump functions
   */
  void clear() {
    NonEmptyReverseLookup.clear();
    NonEmptyForwardLookup.clear();
    NonEmptyLookupByTargetNode.clear();
  }

  void printJumpFunctions(llvm::raw_ostream &OS) {
    OS << "\n******************************************************";
    OS << "\n*              Print all Jump Functions              *";
    OS << "\n******************************************************\n";
    for (auto &Entry : NonEmptyLookupByTargetNode) {
      std::string NLabel = Problem.NtoString(Entry.first);
      OS << "\nN: " << NLabel << "\n---" << std::string(NLabel.size(), '-')
         << '\n';
      for (auto Cell : Entry.second.cellSet()) {
        OS << "D1: " << Problem.DtoString(Cell.r) << '\n'
           << "\tD2: " << Problem.DtoString(Cell.c) << '\n'
           << "\tEF: " << Cell.v->str() << "\n\n";
      }
    }
  }

  void printNonEmptyReverseLookup(llvm::raw_ostream &OS) {
    OS << "DUMP nonEmptyReverseLookup\nTable<N, D, std::unordered_map<D, "
          "EdgeFunctionPtrType>>\n";
    auto CellVec = NonEmptyReverseLookup.cellVec();
    for (auto Cell : CellVec) {
      OS << "N : " << Problem.NtoString(Cell.r)
         << "\nD1: " << Problem.DtoString(Cell.c) << '\n';
      for (auto D2ToEF : Cell.v) {
        OS << "D2: " << Problem.DtoString(D2ToEF.first)
           << "\nEF: " << D2ToEF.second->str() << '\n';
      }
      OS << '\n';
    }
  }

  void printNonEmptyForwardLookup(llvm::raw_ostream &OS) {
    OS << "DUMP nonEmptyForwardLookup\nTable<D, N, std::unordered_map<D, "
          "EdgeFunctionPtrType>>\n";
    auto CellVec = NonEmptyForwardLookup.cellVec();
    for (auto Cell : CellVec) {
      OS << "D1: " << Problem.DtoString(Cell.r)
         << "\nN : " << Problem.NtoString(Cell.c) << '\n';
      for (auto D2ToEF : Cell.v) {
        OS << "D2: " << Problem.DtoString(D2ToEF.first)
           << "\nEF: " << D2ToEF.second->str() << '\n';
      }
      OS << '\n';
    }
  }

  void printNonEmptyLookupByTargetNode(llvm::raw_ostream &OS) {
    OS << "DUMP nonEmptyLookupByTargetNode\nstd::unordered_map<N, Table<D, D, "
          "EdgeFunctionPtrType>>\n";
    for (auto Node : NonEmptyLookupByTargetNode) {
      OS << "\nN : " << Problem.NtoString(Node.first) << '\n';
      auto Table = NonEmptyLookupByTargetNode[Node.first];
      auto CellVec = Table.cellVec();
      for (auto Cell : CellVec) {
        OS << "D1: " << Problem.DtoString(Cell.r)
           << "\nD2: " << Problem.DtoString(Cell.c) << "\nEF: " << Cell.v->str()
           << '\n';
      }
      OS << '\n';
    }
  }
};

} // namespace psr

#endif
