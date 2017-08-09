#ifndef HEXASTORE_HH_
#define HEXASTORE_HH_

#include <iostream>
#include <array>
#include <vector>
#include <string>
#include <sqlite3.h>
#include <iostream>
#include "Queries.hh"
#include <boost/format.hpp>

using namespace std;

namespace hexastore
{
/**
 * @brief Holds the results of query to the hexastore.
 */
struct hs_result {
    string subject;
    string predicate;
    string object;
    hs_result() = default;
    hs_result(string s, string p, string o) : subject(s), predicate(p), object(o) {}
    friend ostream& operator<< (ostream& os, const hs_result& hsr) {
    	return os << "[ subject: " << hsr.subject << " | predicate: " << hsr.predicate << " | object: " << hsr.object << " ]";
    }
};
/**
 * @brief Efficient data structure for holding graphs in databases.
 *
 * This approach is based on the paper "Database-Backed Program Analysis for Scalable
 * Error Propagation" by Weiss, Rubio-González and Libit.
 *
 * To store a graph into a database, we represent the graph by a set of triples
 * by the form:
 *
 *      { <source node> , <edge> , <destination node> }
 *
 *
 * A hexastore indexes graphs six-fold, according to each of the permutations
 * of
 */
class Hexastore {
private:
  sqlite3* hs_internal_db;
  static int callback(void *NotUsed, int argc, char **argv, char **azColName);
  void doPut(string query, array<string, 3> edge);

public:
  /**
   * @brief Constructs a hexastore under the given filename.
   *
   * If the given filename matches an already created hexastore, no
   * new hexastore will be created. Instead the already created hexastore
   * will be used.
   *
   * @param filename Filename of the hexastore.
   */
  Hexastore(string filename);
  ~Hexastore();

  /**
   * @brief Creates a new entry in the hexastore.
   *
   * It is not possible to have duplicate entries in the hexastore and
   * duplicate put queries will be silently ignored by the hexastore.
   *
   * @note To mitigate certain compiler warnings, it is advised to use
   *        double braces, e.g.:
   *        hexastore.put({{"subject", "predicate", "object"}});
   *
   * @param edge Array of strings to be put into the hexastore.
   */
  void put(array<string, 3> edge);

  /**
   *
   * @param edge_query
   * @param result_size_hint
   * @return
   */
  vector<hs_result> get(array<string, 3> edge_query, size_t result_size_hint=0);
};

} /* end namespace hexastore */

#endif /* HEXASTORE_HH_ */
