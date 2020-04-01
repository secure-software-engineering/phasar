/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include "phasar/DB/Hexastore.h"

using namespace psr;
using namespace std;
using namespace boost;

namespace psr {

Hexastore::Hexastore(string filename) {
  sqlite3_open(filename.c_str(), &hs_internal_db);
  const string query = INIT;
  char *err;
  sqlite3_exec(hs_internal_db, query.c_str(), callback, 0, &err);
  if (err != NULL)
    cout << err << "\n\n";
}

Hexastore::~Hexastore() { sqlite3_close(hs_internal_db); }

int Hexastore::callback(void *NotUsed, int argc, char **argv,
                        char **azColName) {
  for (int i = 0; i < argc; ++i) {
    cout << azColName[i] << " " << (argv[i] ? argv[i] : "NULL") << endl;
  }
  return 0;
}

void Hexastore::put(array<string, 3> edge) {
  doPut(SPO_INSERT, edge);
  doPut(SOP_INSERT, edge);
  doPut(PSO_INSERT, edge);
  doPut(POS_INSERT, edge);
  doPut(OSP_INSERT, edge);
  doPut(OPS_INSERT, edge);
}

void Hexastore::doPut(string query, array<string, 3> edge) {
  string compiled_query = str(format(query) % edge[0] % edge[1] % edge[2]);
  char *err;
  sqlite3_exec(hs_internal_db, compiled_query.c_str(), callback, 0, &err);
  if (err != NULL)
    cout << err;
}

vector<hs_result> Hexastore::get(array<string, 3> edge_query,
                                 size_t result_size_hint) {
  vector<hs_result> result;
  result.reserve(result_size_hint);
  string querystring;
  if (edge_query[0] == "?") {
    if (edge_query[1] == "?") {
      if (edge_query[2] == "?") {
        querystring = SEARCH_XXX;
      } else {
        querystring = SEARCH_XXO;
      }
    } else {
      if (edge_query[2] == "?") {
        querystring = SEARCH_XPX;
      } else {
        querystring = SEARCH_XPO;
      }
    }
  } else {
    if (edge_query[1] == "?") {
      if (edge_query[2] == "?") {
        querystring = SEARCH_SXX;
      } else {
        querystring = SEARCH_SXO;
      }
    } else {
      if (edge_query[2] == "?") {
        querystring = SEARCH_SPX;
      } else {
        querystring = SEARCH_SPO;
      }
    }
  }
  string compiled_query =
      str(format(querystring) % edge_query[0] % edge_query[1] % edge_query[2]);
  // this lambda will collect all of our results, since it is called on every
  // row of the result set
  auto sqlite_cb_result_collector = [](void *cb, int argc, char **argv,
                                       char **azColName) {
    vector<hs_result> *res = static_cast<vector<hs_result> *>(cb);
    res->emplace_back(argv[0], argv[1], argv[2]);
    return 0;
  };
  char *err;
  sqlite3_exec(hs_internal_db, compiled_query.c_str(),
               sqlite_cb_result_collector, &result, &err);
  if (err != NULL) {
    cout << err;
  }
  return result;
}

} // namespace psr
