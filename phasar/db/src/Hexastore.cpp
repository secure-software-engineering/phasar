/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/DB/Hexastore.h"

namespace psr {

Hexastore::Hexastore(const std::string &Filename) {
  sqlite3_open(Filename.c_str(), &HSInternalDB);
  const std::string Query = INIT;
  char *Err;
  sqlite3_exec(HSInternalDB, Query.c_str(), callback, nullptr, &Err);
  if (Err != nullptr) {
    llvm::outs() << Err << "\n\n";
  }
}

Hexastore::~Hexastore() { sqlite3_close(HSInternalDB); }

int Hexastore::callback(void * /*NotUsed*/, int Argc, char **Argv,
                        char **AzColName) {
  for (int Idx = 0; Idx < Argc; ++Idx) {
    llvm::outs() << AzColName[Idx] << " " << (Argv[Idx] ? Argv[Idx] : "NULL")
                 << '\n';
  }
  return 0;
}

void Hexastore::put(const std::array<std::string, 3> &Edge) {
  doPut(SPOInsert, Edge);
  doPut(SOPInsert, Edge);
  doPut(PSOInsert, Edge);
  doPut(POSInsert, Edge);
  doPut(OSPInsert, Edge);
  doPut(OPSInsert, Edge);
}

void Hexastore::doPut(const std::string &Query,
                      std::array<std::string, 3> Edge) {
  std::string CompiledQuery =
      str(boost::format(Query) % Edge[0] % Edge[1] % Edge[2]);
  char *Err;
  sqlite3_exec(HSInternalDB, CompiledQuery.c_str(), callback, nullptr, &Err);
  if (Err != nullptr) {
    llvm::outs() << Err;
  }
}

std::vector<HSResult> Hexastore::get(std::array<std::string, 3> EdgeQuery,
                                     size_t ResultSizeHint) {
  std::vector<HSResult> Result;
  Result.reserve(ResultSizeHint);
  std::string QueryString;
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
  std::string CompiledQuery = str(boost::format(QueryString) % EdgeQuery[0] %
                                  EdgeQuery[1] % EdgeQuery[2]);
  // this lambda will collect all of our results, since it is called on every
  // row of the result set
  auto SqliteCBResultCollector = [](void *CB, int /*Argc*/, char **Argv,
                                    char ** /*AzColName*/) {
    auto *Res = static_cast<std::vector<HSResult> *>(CB);
    Res->emplace_back(Argv[0], Argv[1], Argv[2]);
    return 0;
  };
  char *Err;
  sqlite3_exec(HSInternalDB, CompiledQuery.c_str(), SqliteCBResultCollector,
               &Result, &Err);
  if (Err != nullptr) {
    llvm::outs() << Err;
  }
  return Result;
}

} // namespace psr
