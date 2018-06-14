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
#include <initializer_list>
#include <iostream>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
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

namespace psr {

enum class QueryReturnCode { DBTrue, DBFalse, DBError };

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
  const static std::string db_user;
  const static std::string db_password;
  const static std::string db_schema_name;
  const static std::string db_server_address;

  // Functions for internal use only
  int getNextAvailableID(const std::string &TableName);
  int getProjectID(const std::string &Identifier);
  int getModuleID(const std::string &Identifier);
  std::set<int> getModuleIDsFromProject(const std::string &Identifier);
  int getModuleIDFromFunctionID(const unsigned functionID);
  int getModuleIDFromTypeID(const unsigned typeID);
  std::set<int> getFunctionID(const std::string &Identifier);
  std::set<int> getGlobalVariableID(const std::string &Identifier);
  int getTypeID(const std::string &Identifier);
  size_t getFunctionHash(const unsigned functionID);
  size_t getModuleHash(const unsigned moduleID);
  QueryReturnCode moduleHasTypeHierarchy(const unsigned moduleID);
  QueryReturnCode globalVariableIsDeclaration(const unsigned globalVariableID);
  std::set<int> getAllTypeHierarchyIDs();
  std::set<int> getAllModuleIDsFromTH(const unsigned typeHierarchyID);

  bool schemeExists();
  void buildDBScheme();
  void dropDBAndRebuildScheme();

  bool insertModule(const std::string &ProjectIdentifier,
                    const llvm::Module *module);
  std::unique_ptr<llvm::Module> getModule(const std::string &mod_name,
                                          llvm::LLVMContext &Context);
  bool insertGlobalVariable(const llvm::GlobalVariable &G,
                            const unsigned moduleID);
  bool insertFunction(const llvm::Function &F, const unsigned moduleID);
  bool insertType(const llvm::StructType &ST, const unsigned moduleID);
  bool insertVTable(const VTable &VTBL, const std::string &TypeName,
                    const std::string &ProjectName);
  void storeLTHGraphToHex(const LLVMTypeHierarchy::bidigraph_t &G,
                          const std::string hex_id);


public:
  DBConn(const DBConn &db) = delete;
  DBConn(DBConn &&db) = delete;
  DBConn &operator=(const DBConn &db) = delete;
  DBConn &operator=(DBConn &&db) = delete;
  static DBConn &getInstance();
  std::string getDBName();

  void storeProjectIRDB(const std::string &ProjectName,
                        const ProjectIRDB &IRDB);
  ProjectIRDB loadProjectIRDB(const std::string &ProjectName);

  void storeLLVMBasedICFG(const LLVMBasedICFG &ICFG,
                          const std::string &ProjectName, bool use_hs = false);
  LLVMBasedICFG loadLLVMBasedICFGfromModule(const std::string &ModuleName,
                                            bool use_hs = false);
  LLVMBasedICFG
  loadLLVMBasedICFGfromModules(std::initializer_list<std::string> ModuleNames,
                               bool use_hs = false);
  LLVMBasedICFG loadLLVMBasedICFGfromProject(const std::string &ProjectName,
                                             bool use_hs = false);

  void storePointsToGraph(const PointsToGraph &PTG,
                          const std::string &ProjectName, bool use_hs = false);
  PointsToGraph loadPointsToGraphFromFunction(const std::string &FunctionName,
                                              bool use_hs = false);

  void storeLLVMTypeHierarchy(LLVMTypeHierarchy &TH,
                              const std::string &ProjectName,
                              bool use_hs = false);
  LLVMTypeHierarchy
  loadLLVMTypeHierarchyFromModule(const std::string &ModuleName,
                                  bool use_hs = false);
  LLVMTypeHierarchy loadLLVMTypeHierarchyFromModules(
      std::initializer_list<std::string> ModuleNames, bool use_hs = false);
  LLVMTypeHierarchy
  loadLLVMTypeHierarchyFromProject(const std::string &ProjectName,
                                   bool use_hs = false);

  void storeIDESummary(const IDESummary &S);
  IDESummary loadIDESummary(const std::string &FunctionName,
                            const std::string &AnalysisName);
};

} // namespace psr

#endif /* DATABASE_DBCONN_HH_ */
