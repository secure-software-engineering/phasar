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

Hexastore::Hexastore(const string &Filename) {
  sqlite3_open(Filename.c_str(), &HSInternalDB);
  const string Query = INIT;
  char *Err;
  sqlite3_exec(HSInternalDB, Query.c_str(), callback, nullptr, &Err);
  if (Err != nullptr) {
    cout << Err << "\n\n";
  }
}

Hexastore::~Hexastore() { sqlite3_close(HSInternalDB); }

int Hexastore::callback(void * /*NotUsed*/, int Argc, char **Argv,
                        char **AzColName) {
  for (int Idx = 0; Idx < Argc; ++Idx) {
    cout << AzColName[Idx] << " " << (Argv[Idx] ? Argv[Idx] : "NULL") << endl;
  }
  return 0;
}

void Hexastore::put(const array<string, 3> &Edge) {
  doPut(SPOInsert, Edge);
  doPut(SOPInsert, Edge);
  doPut(PSOInsert, Edge);
  doPut(POSInsert, Edge);
  doPut(OSPInsert, Edge);
  doPut(OPSInsert, Edge);
}

void Hexastore::doPut(const string &Query, array<string, 3> Edge) {
  string CompiledQuery = str(format(Query) % Edge[0] % Edge[1] % Edge[2]);
  char *Err;
  sqlite3_exec(HSInternalDB, CompiledQuery.c_str(), callback, nullptr, &Err);
  if (Err != nullptr) {
    cout << Err;
  }
}

vector<HSResult> Hexastore::get(array<string, 3> EdgeQuery,
                                size_t ResultSizeHint) {
  vector<HSResult> Result;
  Result.reserve(ResultSizeHint);
  string QueryString;
  if (EdgeQuery[0] == "?") {
    if (EdgeQuery[1] == "?") {
      if (EdgeQuery[2] == "?") {
        QueryString = SearchXXX;
      } else {
        QueryString = SearchXXO;
      }
    } else {
      if (EdgeQuery[2] == "?") {
        QueryString = SearchXPX;
      } else {
        QueryString = SearchXPO;
      }
    }
  } else {
    if (EdgeQuery[1] == "?") {
      if (EdgeQuery[2] == "?") {
        QueryString = SearchSXX;
      } else {
        QueryString = SearchSXO;
      }
    } else {
      if (EdgeQuery[2] == "?") {
        QueryString = SearchSPX;
      } else {
        QueryString = SearchSPO;
      }
    }
  }
  string CompiledQuery =
      str(format(QueryString) % EdgeQuery[0] % EdgeQuery[1] % EdgeQuery[2]);
  // this lambda will collect all of our results, since it is called on every
  // row of the result set
  auto SqliteCBResultCollector = [](void *CB, int /*Argc*/, char **Argv,
                                    char ** /*AzColName*/) {
    auto *Res = static_cast<vector<HSResult> *>(CB);
    Res->emplace_back(Argv[0], Argv[1], Argv[2]);
    return 0;
  };
  char *Err;
  sqlite3_exec(HSInternalDB, CompiledQuery.c_str(), SqliteCBResultCollector,
               &Result, &Err);
  if (Err != nullptr) {
    cout << Err;
  }
  return Result;
}

} // namespace psr
