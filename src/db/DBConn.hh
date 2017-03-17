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
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "../utils/utils.hh"
#include "ProjectIRCompiledDB.hh"

using namespace std;

struct ResultSet {
  size_t rows = 0;
  vector<string> header;
  vector<vector<string>> data;
};

class DBConn {
 private:
  DBConn(const string name = "llheros_analyzer.db");
  ~DBConn();
  sqlite3* db;
  const string dbname;
  int last_retcode;
  char* error_msg = 0;
  static int resultSetCallBack(void* data, int argc, char** argv,
                               char** azColName);
  void DBInitializationAction();

 public:
  DBConn(const DBConn& db) = delete;
  DBConn(DBConn&& db) = delete;
  DBConn& operator=(const DBConn& db) = delete;
  DBConn& operator=(DBConn&& db) = delete;
  static DBConn& getInstance();
  ResultSet execute(const string& query);
  string getDBName();
  int getLastRetCode();
  string getLastErrorMsg();
  void createDBSchemeForAnalysis(const string& analysis_name);
  bool tableExists(const string& table_name);
  bool dropTable(const string& table_name);
  vector<string> getAllTables();

  // API for querying the IR Modules that correspond to the project under
  // analysis
  bool containsIREntry(string mod_name);
  bool insertIR(const llvm::Module* module);
  unique_ptr<llvm::Module> getIR(string mod_name, llvm::LLVMContext& Context);
  friend void operator<<(DBConn& db, const ProjectIRCompiledDB& irdb);
  size_t getSourceHash(string mod_name);
  size_t getIRHash(string mod_name);
  set<string> getAllIRModuleNames();

  // API for querying points-to information

  // API for querying call-graph information

  // API for querying IFDS/IDE information
};

#endif /* DATABASE_DBCONN_HH_ */
