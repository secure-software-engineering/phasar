/*
 * DBConn.h
 *
 *  Created on: 23.08.2016
 *      Author: pdschbrt
 */

#ifndef DATABASE_DBCONN_H_
#define DATABASE_DBCONN_H_

#include "../analysis/points-to/LLVMTypeHierarchy.h"
// #include "../analysis/points-to/PointsToGraph.h"
#include "../analysis/control_flow/LLVMBasedICFG.h"
#include "../analysis/ifds_ide/IDESummary.h"
#include "../analysis/points-to/VTable.h"
#include "../utils/IO.h"
#include "../utils/utils.h"
#include "Hexastore.h"
// #include "ProjectIRDB.h"
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
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <memory>
#include <mysql_connection.h>
#include <set>
#include <sqlite3.h>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

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
  int getFunctionID(const string &Identifier);
  int getGlobalVariableID(const string &Identifier);
  int getTypeID(const string &Identifier);

  bool schemeExists();
  void buildDBScheme();
  void dropDBAndRebuildScheme();

  bool insertModule(const string &ProjectIdentifier,
                    const llvm::Module *module);
  unique_ptr<llvm::Module> getModule(const string &mod_name,
                                     llvm::LLVMContext &Context);

public:
  DBConn(const DBConn &db) = delete;
  DBConn(DBConn &&db) = delete;
  DBConn &operator=(const DBConn &db) = delete;
  DBConn &operator=(DBConn &&db) = delete;
  static DBConn &getInstance();
  string getDBName();

  void storeProjectIRDB(const string &ProjectName, const ProjectIRDB &IRDB);
  ProjectIRDB loadProjectIRDB(const string &ProjectName);

  void storeLLVMBasedICFG(const LLVMBasedICFG &ICFG, bool use_hs = false);
  LLVMBasedICFG loadLLVMBasedICFGfromModule(const string &ModuleName,
                                            bool use_hs = false);
  LLVMBasedICFG
  loadLLVMBasedICFGfromModules(initializer_list<string> ModuleNames,
                               bool use_hs = false);
  LLVMBasedICFG loadLLVMBasedICFGfromProject(const string &ProjectName,
                                             bool use_hs = false);

  void storePointsToGraph(const PointsToGraph &PTG, bool use_hs = false);
  PointsToGraph loadPointsToGraphFromFunction(const string &FunctionName,
                                              bool use_hs = false);

  void storeLLVMTypeHierarchy(const LLVMTypeHierarchy &TH, bool use_hs = false);
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

#endif /* DATABASE_DBCONN_HH_ */
