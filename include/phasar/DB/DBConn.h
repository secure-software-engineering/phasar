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
/*
#ifndef PHASAR_DB_DBCONN_H_
#define PHASAR_DB_DBCONN_H_

#include <initializer_list>
#include <iostream> // Because of SQL_STD_ERROR_HANDLING
#include <memory>
#include <set>
#include <string>

#include "llvm/IR/Module.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDESummary.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToGraph.h"
#include "phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h"
// If ProjectIRDB is no more returned, forward declare it and remove this
#include "phasar/DB/ProjectIRDB.h"

#include "mysql_connection.h"
#include "sqlite3.h"

namespace llvm {
class GlobalVariable;
class LLVMContext;
class StructType;
} // namespace llvm

namespace psr {

enum class QueryReturnCode { DBTrue, DBFalse, DBError };

#define SQL_STD_ERROR_HANDLING                                                 \
  std::cout << "# ERR: SQLException in " << __FILE__;                          \
  std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;   \
  std::cout << "# ERR: " << e.what();                                          \
  std::cout << " (MySQL error code: " << e.getErrorCode();                     \
  std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;

// forward declarations
class VTable;

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
  std::size_t getFunctionHash(const unsigned functionID);
  std::size_t getModuleHash(const unsigned moduleID);
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
  // We may want to pass an empty ProjectIRDB and do not return anything in
  // order to suppress the copy constructor of ProjectIRDB and enforce a no copy
  // rule.
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

  void storePointsToGraph(const LLVMPointsToGraph &PTG,
                          const std::string &ProjectName, bool use_hs = false);
  LLVMPointsToGraph loadPointsToGraphFromFunction(const std::string
&FunctionName, bool use_hs = false);

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

#endif
*/