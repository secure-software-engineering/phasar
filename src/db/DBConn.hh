/*
 * DBConn.hh
 *
 *  Created on: 23.08.2016
 *      Author: pdschbrt
 */

#ifndef DATABASE_DBCONN_HH_
#define DATABASE_DBCONN_HH_

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <sqlite3.h>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "../analysis/call-points-to_graph/LLVMStructTypeHierarchy.hh"
#include "../analysis/call-points-to_graph/VTable.hh"
#include "../utils/utils.hh"
#include "../utils/IO.hh"
#include "ProjectIRCompiledDB.hh"

#define CPREPARE(FUNCTION)                             \
  if (SQLITE_OK != FUNCTION) {                         \
    cout << "DB error: could not prepare statement\n"; \
    HEREANDNOW;                                        \
  }

#define CBIND(FUNCTION)                             \
  if (SQLITE_OK != FUNCTION) {                      \
    cout << "DB error: could not bind statement\n"; \
    HEREANDNOW;                                     \
  }

#define CSTEP(FUNCTION)                            \
  {                                                \
    int ret = FUNCTION;                            \
    if (ret != SQLITE_DONE || ret != SQLITE_ROW) { \
    }                                              \
  }

#define CRESET(FUNCTION)                \
  if (SQLITE_OK != FUNCTION) {          \
    cout << "DB error: reset failed\n"; \
    HEREANDNOW;                         \
  }

#define CFINALIZE(FUNCTION)               \
  if (SQLITE_OK != FUNCTION) {            \
    cout << "DB error: fialize failed\n"; \
    HEREANDNOW;                           \
  }

using namespace std;

class LLVMStructTypeHierarchy;

// struct ResultSet {
//   size_t rows = 0;
//   vector<string> header;
//   vector<vector<string>> data;
// };

class DBConn {
 private:
  DBConn();
  ~DBConn();
  sqlite3* db;
  int last_retcode;
  char* error_msg = 0;
  // static int resultSetCallBack(void* data, int argc, char** argv,
  //                              char** azColName);
  // Functions for internal use only
  void DBInitializationAction();
  int getModuleID(const string& mod_name);
  int getFunctionID(const string& f_name);
  int getGlobalVariableID(const string& g_name);
  int getTypeID(const string& t_name);

 public:
  static const string dbname;
  DBConn(const DBConn& db) = delete;
  DBConn(DBConn&& db) = delete;
  DBConn& operator=(const DBConn& db) = delete;
  DBConn& operator=(DBConn&& db) = delete;
  static DBConn& getInstance();
  void execute(const string& query);
  string getDBName();
  int getLastRetCode();
  string getLastErrorMsg();

  // API for querying the IR Modules ------------------------------------------
  // Functions for storing information away
  bool insertIRModule(const llvm::Module* module);
  bool insertFunctionModuleDefinition(const string& f_name, const string& mod_name);
  bool insertGlobalVariableModuleDefinition(const string& g_name, const string& mod_name);

  // Functions for re-storing information from db
  set<string> getAllModuleIdentifiers();

  // Functions for querying the database for information
  bool containsIREntry(const string& mod_name);
  unique_ptr<llvm::Module> getIRModule(const string& mod_name, llvm::LLVMContext& Context);
  string getModuleFunctionDefinition(const string& f_name);
  string getGlobalVariableDefinition(const string& g_name);
  size_t getSRCHash(const string& mod_name);
  size_t getIRHash(const string& mod_name);

  // high level load / store functions
  friend void operator<<(DBConn& db, const ProjectIRCompiledDB& irdb);
  friend void operator>>(DBConn& db, const ProjectIRCompiledDB& irdb);
  // --------------------------------------------------------------------------


  // API for querying the class hierarchy information -------------------------
  bool insertType(const string& type_name, VTable);
  VTable getVTable(const string& type_name);
  set<string> getAllTypeIdentifiers();
  // bool insertLLVMStructHierarchyGraph(LLVMStructTypeHierarchy::digraph_t graph);
  // LLVMStructTypeHierarchy::digraph_t getLLVMStructTypeHierarchyGraph();
  friend void operator<<(DBConn& db, const LLVMStructTypeHierarchy& STH);
  friend void operator>>(DBConn& db, const LLVMStructTypeHierarchy& STH);
  // --------------------------------------------------------------------------


  // API for querying points-to information -----------------------------------
  // --------------------------------------------------------------------------


  // API for querying call-graph information ----------------------------------
  // --------------------------------------------------------------------------


  // API for querying IFDS/IDE information ------------------------------------
  // --------------------------------------------------------------------------
};

#endif /* DATABASE_DBCONN_HH_ */
