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
#include "Hexastore.hh"
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
    cout << "DB error: finalize failed\n"; \
    HEREANDNOW;                           \
  }

using namespace std;

class LLVMStructTypeHierarchy;

// struct ResultSet {
//   size_t rows = 0;
//   vector<string> header;
//   vector<vector<string>> data;
// };

/**
 * 	@brief Owns the database and serves an interface for accessing the database.
 *
 *	Database holds the following components of an analyzed program:
 *		- IR Modules
 *		- Global variables
 *		- Functions
 *		- Types
 *
 *	Graphs like Type-Hierarchy-Graph, Points-to-Graph etc. are stored in
 *	a specially foreseen data structure called Hexastore.
 *
 *	The concrete database model can be found in //db_model.
 */
class DBConn {
 private:

	/**
	 * 	@brief Initializes the database at the given path.
	 * 	@param dbname Path to the database.
	 */
  DBConn(const string dbname = "llheros_analyzer.db");
  ~DBConn();
  // Object representing the SQLite3 database.
  sqlite3* db;
  // Name of the database.
  const string dbname;
  // Holds last return code from the database.
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
  DBConn(const DBConn& db) = delete;
  DBConn(DBConn&& db) = delete;
  DBConn& operator=(const DBConn& db) = delete;
  DBConn& operator=(DBConn&& db) = delete;

  /**
   * 	@brief Returns the DBConn object.
   *	@return DBConn object.
   * 	Guarantees that only one instance of DBConn exists.
   */
  static DBConn& getInstance();

  /**
   * 	@brief Executes a SQL query.
   * 	@param query SQL query.
   */
  void execute(const string& query);

  /**
   * 	@brief Returns name of the database.
   * 	@return DB name.
   */
  string getDBName();

  /**
   * 	@brief Returns last return code of the SQLite3 database.
   * 	@return Return code.
   */
  int getLastRetCode();

  /**
   * 	@brief Returns last error message, if there is one.
   * 	@return Error message.
   */
  string getLastErrorMsg();

  // API for querying the IR Modules ------------------------------------------
  // Functions for storing information away

  /**
   * 	@brief Stores the given LLVM module to the database.
   * 	@param module LLVM module.
   * 	@return false?
   */
  bool insertIRModule(const llvm::Module* module);

  /**
   * 	@brief Stores a function to the database.
   * 	@param f_name Function identifier.
   * 	@param mod_name LLVM module identifier in which the function
   * 	                is defined.
   * 	@return false?
   *
   * 	Only the function identifier is needed to be stored. The actual
   * 	function can be reconstructed from the LLVM Module it is
   * 	defined in.
   */
  bool insertFunctionModuleDefinition(const string& f_name,
                                      const string& mod_name);

  /**
   * 	@brief Stores a global variable to the database.
   * 	@param g_name Global variable identifier.
   * 	@param mod_name LLVM module identifier in which the global variable
   * 	                is defined.
   * 	@return false?
   *
   * 	Only the global variable identifier is needed to be stored.
   * 	The actual global variable can be reconstructed from the LLVM Module
   * 	it is defined in.
   */
  bool insertGlobalVariableModuleDefinition(const string& g_name,
                                            const string& mod_name);

  // Functions for re-storing information from db

  /**
   * 	@brief Retrieves all LLVM module identifiers from database.
   * 	@return Set of LLVM module identifiers.
   */
  set<string> getAllModuleIdentifiers();

  // Functions for querying the database for information
  /**
   * 	@brief Checks if an IR module with the given name is stored in
   * 	       the database.
   * 	@param mod_name IR module identifier.
   * 	@return True, if IR module is stored in the database.
   * 	        False, otherwise.
   */
  bool containsIREntry(const string& mod_name);

  /**
   * 	@brief Retrieves IR module with given name from database.
   * 	@param mod_name IR module identifier.
   * 	@param Context LLVM Context belonging to the IR module.
   * 	@return Pointer (unique_ptr) to retrieved IR module.
   */
  unique_ptr<llvm::Module> getIRModule(const string& mod_name,
                                       llvm::LLVMContext& Context);

  /**
   * 	@brief Retrieves IR module identifier, which defines the given
   * 	       function, from database.
   * 	@param f_name Function identifier.
   * 	@return IR module identifier.
   */
  string getModuleFunctionDefinition(const string& f_name);

  /**
   * 	@brief Retrieves IR module identifier, which defines the given
   * 	       global variable, from database.
   * 	@param g_name Global variable identifier.
   * 	@return IR module identifier.
   */
  string getGlobalVariableDefinition(const string& g_name);

  /**
   * 	@brief Retrieves the hash value of the source file corresponding
   * 	       to the given IR module.
   * 	@param mod_name IR module identifier.
   * 	@return Hash value of source file.
   */
  size_t getSRCHash(const string& mod_name);

  /**
   * 	@brief Retrieves the hash value of the given IR module.
   * 	@param mod_name IR module identifier.
   * 	@return Hash value of IR module.
   */
  size_t getIRHash(const string& mod_name);

  // high level load / store functions

  /**
   * 	@brief ProjectIRCompiledDB store operator.
   * 	@param db Serves as an interface for accessing the database.
   * 	@param irdb Reference to the ProjectIRCompiledDB object that
   * 	            is stored.
   *
   * 	Stores all information of the ProjectIRCompiledDB in the database
   * 	which needs to be stored persistently.
   */
  friend void operator<<(DBConn& db, const ProjectIRCompiledDB& irdb);

  /**
   * 	@brief ProjectIRCompiledDB load/re-store operator.
   * 	@param db Serves as an interface for accessing the database.
   * 	@param irdb ProjectIRCompileDB object that will hold the
   * 	            restored information.
   */
  friend void operator>>(DBConn& db, ProjectIRCompiledDB& irdb);
  // --------------------------------------------------------------------------

  // API for querying the class hierarchy information -------------------------

  /**
   * 	@brief Stores a virtual types in the database.
   * 	@param type_name Name of the type that is stored.
   * 	@param vtbl Virtual method table of the given type.
   * 	@return false?
   */
  bool insertType(const string& type_name, VTable vtbl);

  /**
   * 	@brief Stores a non-virtual type in the database.
   * 	@param type_name Name of the type that is stored.
   *	@return false?
   *
   * 	A non-virtual class/struct has no virtual functions and does not inherit
   * 	from any other class/struct, hence there is no virtual method table.
   */
  bool insertType(const string& type_name);

  /**
   * 	@brief Retrieves the virtual method table of the given type from database.
   * 	@param type_name Name of the type.
   * 	@return An object of the virtual method table.
   */
  VTable getVTable(const string& type_name);

  /**
   * 	@brief Retrieves all type identifiers from database.
   * 	@return Set of all type identifiers.
   */
  set<string> getAllTypeIdentifiers();

  /**
   * 	@brief Retrieves the function identifier belonging to the given function
   *         id from database.
   * 	@Return Function identifier.
   */
  string getFunctionIdentifier(int type_id);

  // bool insertLLVMStructHierarchyGraph(LLVMStructTypeHierarchy::digraph_t graph);
  // LLVMStructTypeHierarchy::digraph_t getLLVMStructTypeHierarchyGraph();

  /**
   * 	@brief LLVMStructTypeHierarchy store operator.
   * 	@param db Serves as an interface for accessing the database.
   * 	@param STH LLVMStructTypeHierarchy object that is stored.
   *
   * 	By storing the class hierarchy in the database, a repeated
   * 	reconstruction of the class hierarchy graph as well as the
   * 	VTables from the corresponding LLVM module(s) is unnecessary.
   *
   * 	To store the class hierarchy graph itself, a Hexastore data
   * 	structure is used.
   */
  friend void operator<<(DBConn& db, const LLVMStructTypeHierarchy& STH);

  /**
   * 	@brief LLVMStructTypeHierarchy load/re-store operator.
   * 	@param db Serves as an interface for accessing the database.
   * 	@param STH LLVMStructTypeHierarchy object that is re-stored.
   */
  friend void operator>>(DBConn& db, LLVMStructTypeHierarchy& STH);
  // --------------------------------------------------------------------------

  // API for querying points-to information -----------------------------------
  // --------------------------------------------------------------------------

  // API for querying call-graph information ----------------------------------
  // --------------------------------------------------------------------------

  // API for querying IFDS/IDE information ------------------------------------
  // --------------------------------------------------------------------------
};

#endif /* DATABASE_DBCONN_HH_ */
