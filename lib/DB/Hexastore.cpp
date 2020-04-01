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

Hexastore::Hexastore(string Filename) {
  sqlite3_open(Filename.c_str(), &hs_internal_db);
  const string query = INIT;
  char *err;
  sqlite3_exec(hs_internal_db, query.c_str(), callback, 0, &err);
  if (err != NULL)
    cout << err << "\n\n";
}

Hexastore::~Hexastore() { sqlite3_close(hs_internal_db); }

int Hexastore::callback(void *NotUsed, int Argc, char **Argv,
                        char **AzColName) {
  for (int i = 0; i < Argc; ++i) {
    cout << AzColName[i] << " " << (Argv[i] ? Argv[i] : "NULL") << endl;
  }
  return 0;
}

void Hexastore::put(array<string, 3> Edge) {
  doPut(SPO_INSERT, Edge);
  doPut(SOP_INSERT, Edge);
  doPut(PSO_INSERT, Edge);
  doPut(POS_INSERT, Edge);
  doPut(OSP_INSERT, Edge);
  doPut(OPS_INSERT, Edge);
}

void Hexastore::doPut(string Query, array<string, 3> Edge) {
  string compiled_query = str(format(Query) % Edge[0] % Edge[1] % Edge[2]);
  char *err;
  sqlite3_exec(hs_internal_db, compiled_query.c_str(), callback, 0, &err);
  if (err != NULL)
    cout << err;
}

vector<hs_result> Hexastore::get(array<string, 3> EdgeQuery,
                                 size_t ResultSizeHint) {
  vector<hs_result> result;
  result.reserve(ResultSizeHint);
  string querystring;
  if (EdgeQuery[0] == "?") {
    if (EdgeQuery[1] == "?") {
      if (EdgeQuery[2] == "?") {
        querystring = SEARCH_XXX;
      } else {
        querystring = SEARCH_XXO;
      }
    } else {
      if (EdgeQuery[2] == "?") {
        querystring = SEARCH_XPX;
      } else {
        querystring = SEARCH_XPO;
      }
    }
  } else {
    if (EdgeQuery[1] == "?") {
      if (EdgeQuery[2] == "?") {
        querystring = SEARCH_SXX;
      } else {
        querystring = SEARCH_SXO;
      }
    } else {
      if (EdgeQuery[2] == "?") {
        querystring = SEARCH_SPX;
      } else {
        querystring = SEARCH_SPO;
      }
    }
  }
  string compiled_query =
      str(format(querystring) % EdgeQuery[0] % EdgeQuery[1] % EdgeQuery[2]);
  // this lambda will collect all of our results, since it is called on every
  // row of the result set
  auto sqlite_cb_result_collector = [](void *CB, int Argc, char **Argv,
                                       char **AzColName) {
    vector<hs_result> *res = static_cast<vector<hs_result> *>(CB);
    res->emplace_back(Argv[0], Argv[1], Argv[2]);
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
