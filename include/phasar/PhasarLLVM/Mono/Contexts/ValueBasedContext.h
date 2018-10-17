/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Nicolas Bellec and others
 *****************************************************************************/

/*
 * ValueBasedContext.h
 *
 *  Created on: 19.06.2018
 *      Author: nicolas
 */

// NB: Generic implementation, could be far more precise if the analysis
// takes into account the variables as we can reduce the number of
// args copied to match exactly the function arguments + the global variables
// used by the function (+ static variables ?)

#ifndef PHASAR_PHASARLLVM_MONO_CONTEXTS_VALUEBASEDCONTEXT_H_
#define PHASAR_PHASARLLVM_MONO_CONTEXTS_VALUEBASEDCONTEXT_H_

#include <map>
#include <ostream>

#include <phasar/PhasarLLVM/Mono/Contexts/ContextBase.h>

namespace psr {

/**
 * The value based context represents the context of a function by the
 * values that are used as parameters of the function. Beside the current
 * arguments, the context state also stores the arguments of the previous
 * function on the call stack.
 *
 * @tparam N node in the ICFG
 * @tparam D domain of the analysis (and the function arguments)
 */
template <typename N, typename D>
class ValueBasedContext : public ContextBase<N, D, ValueBasedContext<N, D>> {
public:
  using Node_t = N;
  using Domain_t = D;

protected:
  Domain_t args;
  Domain_t prev_context;

public:
  ValueBasedContext(const NodePrinter<N> *np, const DataFlowFactPrinter<D> *dp)
      : ContextBase<N, D, ValueBasedContext<N, D>>(np, dp) {}

  void enterFunction(const Node_t src, const Node_t dest,
                     const Domain_t &In) override {
    prev_context.swap(args); // O(1)
    args = In;               // O(|In|)
  }

  void exitFunction(const Node_t src, const Node_t dest,
                    const Domain_t &In) override {
    args.swap(prev_context); // O(1)
    prev_context.clear();    // O(|prev_context|)
  }

  bool isUnsure() override { return false; }

  bool isEqual(const ValueBasedContext &rhs) const override {
    if (rhs.args.size() != args.size())
      return false;

    return std::equal(args.cbegin(), args.cend(), rhs.args.cbegin());
  }

  bool isDifferent(const ValueBasedContext &rhs) const override {
    return !isEqual(rhs);
  }

  bool isLessThan(const ValueBasedContext &rhs) const override {
    if (args.size() < rhs.args.size())
      return true;

    auto lhs_it = args.cbegin();
    const auto lhs_end = args.cend();

    auto rhs_it = rhs.args.cbegin();
    const auto rhs_end = rhs.args.cend();

    while (lhs_it != lhs_end && rhs_it != rhs_end) {
      if (*lhs_it < *rhs_it)
        return true;
      if (*lhs_it > *rhs_it)
        return false;
      ++lhs_it;
      ++rhs_it;
    }

    return false;
  }

  Domain_t getArgs() { return args; }

  Domain_t getPrevContext() { return prev_context; }

  void print(std::ostream &os) const override {
    // TODO Implement print for ValueBasedContext
    os << "print not yet implemented, sorry!";
  }
};

/**
 * @tparam Node node in the ICFG
 * @tparam Key key type of the map
 * @tparam Value value type of the map
 * @tparam IdGenerator generator of Id from the source and destination (in that
 * particular order)
 * @tparam Map partial type of Map used. The full type will be Map<Key, Value>
 */
template <typename Node, typename Key, typename Value, typename IdGenerator,
          template <class, class, class...> class Map = std::map>
class MappedValueBasedContext
    : public ContextBase<
          Node, Map<Key, Value>,
          MappedValueBasedContext<Node, Key, Value, IdGenerator, Map>> {
public:
  using Node_t = Node;
  using Key_t = Key;
  using Value_t = Value;
  using IdGen_t = IdGenerator;

  using Domain_t = Map<Key_t, Value_t>;

protected:
  Domain_t args;
  Domain_t prev_context;
  IdGen_t IdGen;

public:
  MappedValueBasedContext(const NodePrinter<Node> *np,
                          const DataFlowFactPrinter<Domain_t> *dp)
      : ContextBase<
            Node, Map<Key, Value>,
            MappedValueBasedContext<Node, Key, Value, IdGenerator, Map>>(np,
                                                                         dp) {}

  MappedValueBasedContext(const NodePrinter<Node> *np,
                          const DataFlowFactPrinter<Domain_t> *dp,
                          IdGen_t &_IdGen)
      : ContextBase<
            Node, Map<Key, Value>,
            MappedValueBasedContext<Node, Key, Value, IdGenerator, Map>>(np,
                                                                         dp),
        IdGen(_IdGen) {}

  template <class T>
  MappedValueBasedContext(T &&_args, T &&_prev_context)
      : args(std::forward<T>(_args)),
        prev_context(std::forward<T>(_prev_context)) {}

  void enterFunction(const Node_t src, const Node_t dest,
                     const Domain_t &In) override {
    prev_context.swap(args); // O(1)

    args.clear(); // O(|args|)
    auto ids = SrcIdGen(src, dest);

    for (auto id : ids) {
      if (In.count(id) == 0) {
        // An argument is not in the given entry, check if the analysis doesn't
        // become unsound
      }
      args[id] = In[id];
    }
  }

  void exitFunction(const Node_t src, const Node_t dest,
                    const Domain_t &In) override {
    args.swap(prev_context); // O(1)
    prev_context.clear();    // O(|prev_context|)
  }

  bool isUnsure() override { return false; }

  bool isEqual(const MappedValueBasedContext &rhs) const override {
    if (rhs.args.size() != args.size())
      return false;

    return std::equal(args.cbegin(), args.cend(), rhs.args.cbegin());
  }

  bool isDifferent(const MappedValueBasedContext &rhs) const override {
    return !isEqual(rhs);
  }

  bool isLessThan(const MappedValueBasedContext &rhs) const override {
    if (args.size() < rhs.args.size())
      return true;

    auto lhs_it = args.cbegin();
    const auto lhs_end = args.cend();

    auto rhs_it = rhs.args.cbegin();
    const auto rhs_end = rhs.args.cend();

    while (lhs_it != lhs_end && rhs_it != rhs_end) {
      if (*lhs_it < *rhs_it)
        return true;
      if (*lhs_it > *rhs_it)
        return false;
      ++lhs_it;
      ++rhs_it;
    }

    return false;
  }

  void print(std::ostream &os) const override {
    // TODO Implement print for MappedValueBasedContext
    os << "print not yet implemented, sorry!";
  }
};

} // namespace psr

#endif
