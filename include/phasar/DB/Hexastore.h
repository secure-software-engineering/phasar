/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Christian Stritzke, Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_DB_HEXASTORE_H_
#define PHASAR_DB_HEXASTORE_H_

#include "phasar/Config/phasar-config.h"
#ifndef PHASAR_HAS_SQLITE
#error                                                                         \
    "Hexastore requires SQLite3. Please install libsqlite3-dev and reconfigure PhASAR."
#endif

#include "llvm/Support/raw_ostream.h"

#include <array>
#include <string>
#include <vector>

struct sqlite3;

namespace psr {
/**
 * @brief Holds the results of a query to the Hexastore.
 */
struct HSResult {
  /// Used for the source node.
  std::string Subject;
  /// Used for the edge.
  std::string Predicate;
  /// Used for the destination node.
  std::string Object;
  HSResult() = default;
  HSResult(const std::string Subject, // NOLINT
           std::string Predicate,     // NOLINT
           std::string Object)        // NOLINT
      : Subject(Subject), Predicate(std::move(Predicate)),
        Object(std::move(Object)) {}
  /// Prints an entry of the results to the command-line
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const HSResult &Result) {
    return OS << "[ subject: " << Result.Subject
              << " | predicate: " << Result.Predicate
              << " | object: " << Result.Object << " ]";
  }

  friend bool operator==(const HSResult &LHS, const HSResult &RHS) {
    return LHS.Subject == RHS.Subject && LHS.Predicate == RHS.Predicate &&
           LHS.Object == RHS.Object;
  }
};

/**
 * A Hexastore is an efficient approach to store large graphs.
 * This approach is based on the paper "Database-Backed Program Analysis
 * for Scalable Error Propagation" by Weiss, Rubio-González and Libit.
 *
 * To store a graph into a database, we represent the graph by a set of
 * string 3-tuples by the form:
 *
 *          (source node, edge, destination node)
 *
 * A Hexastore indexes and saves graphs six-fold, according to each of
 * the permutations of source, edge and destination label. For example,
 * this allows to quickly find all edges for a given source node with one
 * look-up. In general, given one or two fixed elements of a (source, edge,
 * destination) tuple, Hexastore can quickly access the related information.
 *
 * @brief Efficient data structure for holding graphs in databases.
 */
class Hexastore {
private:
  sqlite3 *HSInternalDB{};
  static int callback(void * /*NotUsed*/, int Argc, char **Argv,
                      char **AzColName);
  void doPut(const std::string &Query, std::array<std::string, 3> Edge);

public:
  /**
   * If the given filename matches an already created Hexastore, no
   * new Hexastore will be created. Instead the already created Hexastore
   * will be used.
   *
   * @brief Constructs a Hexastore under the given filename.
   * @param filename Filename of the Hexastore.
   */
  Hexastore(const std::string &FileName);
  Hexastore(const Hexastore &) = delete;
  Hexastore &operator=(const Hexastore &) = delete;

  /**
   * Destructor.
   */
  ~Hexastore();

  /**
   * Adds the given tuple as a new entry to the Hexastore. It is not
   * possible to have duplicate entries in the Hexastore and
   * duplicate put queries will be silently ignored by the Hexastore.
   *
   * @brief Creates a new entry in the Hexastore.
   * @note To mitigate certain compiler warnings, it is advised to use
   *        double braces, e.g.:
   *        hexastore.put({{"subject", "predicate", "object"}});
   * @param edge New entry in the form of a 3-tuple.
   */
  void put(const std::array<std::string, 3> &Edge);

  /**
   * A query is always in the form of a 3-tuple (source, edge, destination)
   * where
   * none/one/two or all three elements of the tuple are given, while the not
   * fixed
   * elements are represented by a "?". E.g. to query all destination nodes
   * for a certain source node and edge, the following query is used:
   *
   *    ("node_19", "edge_3", "?")
   *
   * @brief Query information from the Hexastore.
   * @param edge_query Query in the form of a 3-tuple.
   * @param result_size_hint Used for possible optimization.
   * @return An object of hs_result, holding the queried information.
   */
  std::vector<HSResult> get(std::array<std::string, 3> EdgeQuery,
                            size_t ResultSizeHint = 0);
};

} // namespace psr

#endif
