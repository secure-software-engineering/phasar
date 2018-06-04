/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * DBConn.h
 *
 *  Created on: 23.08.2016
 *      Author: pdschbrt
 */

#ifndef DATABASE_DBCONN_H_
#define DATABASE_DBCONN_H_

#include <boost/graph/adjacency_list.hpp>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <memory>
#include <mysql_connection.h>
#include <phasar/DB/Hexastore.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/IDESummary.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/PhasarLLVM/Pointer/VTable.h>
#include <phasar/Utils/IO.h>
#include <phasar/Utils/Macros.h>
#include <set>
#include <sqlite3.h>
#include <sstream>
#include <string>
#include <thread>
#include <typeinfo>
#include <vector>
using namespace std;
namespace psr {

enum class QueryReturnCode { TRUE, FALSE, ERROR };

#define SQL_STD_ERROR_HANDLING                                                 \
  cout << "# ERR: SQLException in " << __FILE__;                               \
  cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;             \
  cout << "# ERR: " << e.what();                                               \
  cout << " (MySQL error code: " << e.getErrorCode();                          \
  cout << ", SQLState: " << e.getSQLState() << " )" << endl;

// forward declarations
class LLVMTypeHierarchy;
class ProjectIRDB;
class PointsToGraph;

class DBConn {
private:
  DBConn();
  ~DBConn();
  sql::Driver *driver;
  sql::Connection *conn;
  const static string db_user;
  const static string db_password;
  const static string db_schema_name;
  const static string db_server_address;

  // Functions for internal use only
  int getNextAvailableID(const string &TableName);
  int getProjectID(const string &Identifier);
  int getModuleID(const string &Identifier);
  set<int> getModuleIDsFromProject(const string &Identifier);
  int getModuleIDFromFunctionID(const unsigned functionID);
  int getModuleIDFromTypeID(const unsigned typeID);
  set<int> getFunctionID(const string &Identifier);
  set<int> getGlobalVariableID(const string &Identifier);
  int getTypeID(const string &Identifier);
  size_t getFunctionHash(const unsigned functionID);
  size_t getModuleHash(const unsigned moduleID);
  QueryReturnCode moduleHasTypeHierarchy(const unsigned moduleID);
  QueryReturnCode globalVariableIsDeclaration(const unsigned globalVariableID);
  set<int> getAllTypeHierarchyIDs();
  set<int> getAllModuleIDsFromTH(const unsigned typeHierarchyID);

  bool schemeExists();
  void buildDBScheme();
  void dropDBAndRebuildScheme();

  bool insertModule(const string &ProjectIdentifier,
                    const llvm::Module *module);
  unique_ptr<llvm::Module> getModule(const string &mod_name,
                                     llvm::LLVMContext &Context);
  bool insertGlobalVariable(const llvm::GlobalVariable &G,
                            const unsigned moduleID);
  bool insertFunction(const llvm::Function &F, const unsigned moduleID);
  bool insertType(const llvm::StructType &ST, const unsigned moduleID);
  bool insertVTable(const VTable &VTBL, const string &TypeName,
                    const string &ProjectName);
  void storeLTHGraphToHex(const LLVMTypeHierarchy::bidigraph_t &G,
                          const string hex_id);

  FRIEND_TEST(StoreProjectIRDBTest, StoreProjectIRDBTest);

public:
  DBConn(const DBConn &db) = delete;
  DBConn(DBConn &&db) = delete;
  DBConn &operator=(const DBConn &db) = delete;
  DBConn &operator=(DBConn &&db) = delete;
  static DBConn &getInstance();
  string getDBName();

  void storeProjectIRDB(const string &ProjectName, const ProjectIRDB &IRDB);
  ProjectIRDB loadProjectIRDB(const string &ProjectName);

  void storeLLVMBasedICFG(const LLVMBasedICFG &ICFG, const string &ProjectName,
                          bool use_hs = false);
  LLVMBasedICFG loadLLVMBasedICFGfromModule(const string &ModuleName,
                                            bool use_hs = false);
  LLVMBasedICFG
  loadLLVMBasedICFGfromModules(initializer_list<string> ModuleNames,
                               bool use_hs = false);
  LLVMBasedICFG loadLLVMBasedICFGfromProject(const string &ProjectName,
                                             bool use_hs = false);

  void storePointsToGraph(const PointsToGraph &PTG, const string &ProjectName,
                          bool use_hs = false);
  PointsToGraph loadPointsToGraphFromFunction(const string &FunctionName,
                                              bool use_hs = false);

  void storeLLVMTypeHierarchy(LLVMTypeHierarchy &TH, const string &ProjectName,
                              bool use_hs = false);
  LLVMTypeHierarchy loadLLVMTypeHierarchyFromModule(const string &ModuleName,
                                                    bool use_hs = false);
  LLVMTypeHierarchy
  loadLLVMTypeHierarchyFromModules(initializer_list<string> ModuleNames,
                                   bool use_hs = false);
  LLVMTypeHierarchy loadLLVMTypeHierarchyFromProject(const string &ProjectName,
                                                     bool use_hs = false);

  void storeIDESummary(const IDESummary &S);
  IDESummary loadIDESummary(const string &FunctionName,
                            const string &AnalysisName);
};

} // namespace psr

#endif /* DATABASE_DBCONN_HH_ */
