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

#ifndef PHASAR_PHASARLLVM_IFDSIDE_SOLVER_JUMPFUNCTIONS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_SOLVER_JUMPFUNCTIONS_H_

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
  EdgeFunctionPtrType allTop;
  const IDETabulationProblem<AnalysisDomainTy, Container> &problem;

protected:
  // mapping from target node and value to a list of all source values and
  // associated functions where the list is implemented as a mapping from
  // the source value to the function we exclude empty default functions
  Table<n_t, d_t, llvm::SmallVector<std::pair<d_t, EdgeFunctionPtrType>, 1>>
      nonEmptyReverseLookup;
  // mapping from source value and target node to a list of all target values
  // and associated functions where the list is implemented as a mapping from
  // the source value to the function we exclude empty default functions
  Table<d_t, n_t, llvm::SmallVector<std::pair<d_t, EdgeFunctionPtrType>, 1>>
      nonEmptyForwardLookup;
  // a mapping from target node to a list of triples consisting of source value,
  // target value and associated function; the triple is implemented by a table
  // we exclude empty default functions
  std::unordered_map<n_t, Table<d_t, d_t, EdgeFunctionPtrType>>
      nonEmptyLookupByTargetNode;

public:
  JumpFunctions(EdgeFunctionPtrType allTop,
                const IDETabulationProblem<AnalysisDomainTy, Container> &p)
      : allTop(std::move(allTop)), problem(p) {}

  ~JumpFunctions() = default;

  JumpFunctions(const JumpFunctions &JFs) = default;
  JumpFunctions &operator=(const JumpFunctions &JFs) = default;
  JumpFunctions(JumpFunctions &&JFs) noexcept = default;
  JumpFunctions &operator=(JumpFunctions &&JFs) noexcept = default;

  /**
   * Records a jump function. The source statement is implicit.
   * @see PathEdge
   */
  void addFunction(d_t sourceVal, n_t target, d_t targetVal,
                   EdgeFunctionPtrType function) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "Start adding new jump function";
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Fact at source : " << problem.DtoString(sourceVal);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Fact at target : " << problem.DtoString(targetVal);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Destination    : " << problem.NtoString(target);
                  BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Edge Function  : " << function->str());
    // we do not store the default function (all-top)
    if (function->equal_to(allTop)) {
      return;
    }

    auto &SourceValToFunc = nonEmptyReverseLookup.get(target, targetVal);
    if (auto Find = std::find_if(
            SourceValToFunc.begin(), SourceValToFunc.end(),
            [sourceVal](const std::pair<d_t, EdgeFunctionPtrType> &Entry) {
              return sourceVal == Entry.first;
            });
        Find != SourceValToFunc.end()) {
      // it is important that existing values in JumpFunctions are overwritten
      Find->second = function;
    } else {
      SourceValToFunc.emplace_back(sourceVal, function);
    }

    auto &TargetValToFunc = nonEmptyForwardLookup.get(sourceVal, target);
    if (auto Find = std::find_if(
            TargetValToFunc.begin(), TargetValToFunc.end(),
            [targetVal](const std::pair<d_t, EdgeFunctionPtrType> &Entry) {
              return targetVal == Entry.first;
            });
        Find != TargetValToFunc.end()) {
      // it is important that existing values in JumpFunctions are overwritten
      Find->second = function;
    } else {
      TargetValToFunc.emplace_back(targetVal, function);
    }

    // V Table::insert(R r, C c, V v) always overrides (see comments above)
    nonEmptyLookupByTargetNode[target].insert(sourceVal, targetVal, function);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                      << "End adding new jump function";
                  BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
  }

  /**
   * Returns, for a given target statement and value all associated
   * source values, and for each the associated edge function.
   * The return value is a mapping from source value to function.
   */
  std::optional<std::reference_wrapper<
      llvm::SmallVector<std::pair<d_t, EdgeFunctionPtrType>, 1>>>
  reverseLookup(n_t target, d_t targetVal) {
    if (!nonEmptyReverseLookup.contains(target, targetVal)) {
      return std::nullopt;
    } else {
      return {nonEmptyReverseLookup.get(target, targetVal)};
    }
  }

  /**
   * Returns, for a given source value and target statement all
   * associated target values, and for each the associated edge function.
   * The return value is a mapping from target value to function.
   */
  std::optional<std::reference_wrapper<
      llvm::SmallVector<std::pair<d_t, EdgeFunctionPtrType>, 1>>>
  forwardLookup(d_t sourceVal, n_t target) {
    if (!nonEmptyForwardLookup.contains(sourceVal, target)) {
      return std::nullopt;
    } else {
      return {nonEmptyForwardLookup.get(sourceVal, target)};
    }
  }

  /**
   * Returns for a given target statement all jump function records with this
   * target.
   * The return value is a set of records of the form
   * (sourceVal,targetVal,edgeFunction).
   */
  Table<d_t, d_t, EdgeFunctionPtrType> lookupByTarget(n_t target) {
    return nonEmptyLookupByTargetNode[target];
  }

  /**
   * Removes a jump function. The source statement is implicit.
   * @see PathEdge
   * @return True if the function has actually been removed. False if it was not
   * there anyway.
   */
  bool removeFunction(d_t sourceVal, n_t target, d_t targetVal) {
    auto &SourceValToFunc = nonEmptyReverseLookup.get(target, targetVal);
    if (auto Find = std::find_if(
            SourceValToFunc.begin(), SourceValToFunc.end(),
            [sourceVal](const std::pair<d_t, EdgeFunctionPtrType> &Entry) {
              return sourceVal == Entry.first;
            });
        Find != SourceValToFunc.end()) {
      SourceValToFunc.erase(Find);
    }
    auto &TargetValToFunc = nonEmptyForwardLookup.get(sourceVal, target);
    if (auto Find = std::find_if(
            TargetValToFunc.begin(), TargetValToFunc.end(),
            [targetVal](const std::pair<d_t, EdgeFunctionPtrType> &Entry) {
              return targetVal == Entry.first;
            });
        Find != TargetValToFunc.end()) {
      TargetValToFunc.erase(Find);
    }
    return nonEmptyLookupByTargetNode.erase(target);
  }

  /**
   * Removes all jump functions
   */
  void clear() {
    nonEmptyReverseLookup.clear();
    nonEmptyForwardLookup.clear();
    nonEmptyLookupByTargetNode.clear();
  }

  void printJumpFunctions(std::ostream &os) {
    os << "\n******************************************************";
    os << "\n*              Print all Jump Functions              *";
    os << "\n******************************************************\n";
    for (auto &entry : nonEmptyLookupByTargetNode) {
      std::string nLabel = problem.NtoString(entry.first);
      os << "\nN: " << nLabel << "\n---" << std::string(nLabel.size(), '-')
         << '\n';
      for (auto cell : entry.second.cellSet()) {
        os << "D1: " << problem.DtoString(cell.r) << '\n'
           << "\tD2: " << problem.DtoString(cell.c) << '\n'
           << "\tEF: " << cell.v->str() << "\n\n";
      }
    }
  }

  void printNonEmptyReverseLookup(std::ostream &os) {
    os << "DUMP nonEmptyReverseLookup\nTable<N, D, std::unordered_map<D, "
          "EdgeFunctionPtrType>>\n";
    auto cellvec = nonEmptyReverseLookup.cellVec();
    for (auto cell : cellvec) {
      os << "N : " << problem.NtoString(cell.r)
         << "\nD1: " << problem.DtoString(cell.c) << '\n';
      for (auto D2ToEF : cell.v) {
        os << "D2: " << problem.DtoString(D2ToEF.first)
           << "\nEF: " << D2ToEF.second->str() << '\n';
      }
      os << '\n';
    }
  }

  void printNonEmptyForwardLookup(std::ostream &os) {
    os << "DUMP nonEmptyForwardLookup\nTable<D, N, std::unordered_map<D, "
          "EdgeFunctionPtrType>>\n";
    auto cellvec = nonEmptyForwardLookup.cellVec();
    for (auto cell : cellvec) {
      os << "D1: " << problem.DtoString(cell.r)
         << "\nN : " << problem.NtoString(cell.c) << '\n';
      for (auto D2ToEF : cell.v) {
        os << "D2: " << problem.DtoString(D2ToEF.first)
           << "\nEF: " << D2ToEF.second->str() << '\n';
      }
      os << '\n';
    }
  }

  void printNonEmptyLookupByTargetNode(std::ostream &os) {
    os << "DUMP nonEmptyLookupByTargetNode\nstd::unordered_map<N, Table<D, D, "
          "EdgeFunctionPtrType>>\n";
    for (auto node : nonEmptyLookupByTargetNode) {
      os << "\nN : " << problem.NtoString(node.first) << '\n';
      auto table = nonEmptyLookupByTargetNode[node.first];
      auto cellvec = table.cellVec();
      for (auto cell : cellvec) {
        os << "D1: " << problem.DtoString(cell.r)
           << "\nD2: " << problem.DtoString(cell.c) << "\nEF: " << cell.v->str()
           << '\n';
      }
      os << '\n';
    }
  }
};

} // namespace psr

#endif
