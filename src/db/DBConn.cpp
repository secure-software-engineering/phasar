/*
 * DBConn.cpp
 *
 *  Created on: 23.08.2016
 *      Author: pdschbrt
 */

#include "DBConn.hh"

const string DBConn::dbname = "llheros_analyzer.db";

DBConn::DBConn() {
  if ((last_retcode = sqlite3_open(dbname.c_str(), &db)) != SQLITE_OK) {
    cerr << "could not open sqlite3 database!" << endl;
    HEREANDNOW;
  }
  DBInitializationAction();
}

DBConn::~DBConn() {
  if ((last_retcode = sqlite3_close(db)) != SQLITE_OK) {
    cerr << "could not close sqlite3 database properly!" << endl;
    HEREANDNOW;
  }
}

void DBConn::synchronize(ProjectIRCompiledDB* irdb) {
	IRDB = irdb;
}

void DBConn::desynchronize() {
	IRDB = nullptr;
}

bool DBConn::isSynchronized() {
	return IRDB != nullptr;
}

int DBConn::getModuleID(const string& mod_name) {
  static string module_id_query = "SELECT ID FROM IR_MODULE WHERE MODULE_IDENTIFIER=?;";
  sqlite3_stmt* module_id_statement;
  CPREPARE(sqlite3_prepare_v2(db, module_id_query.c_str(), -1, &module_id_statement, NULL));
  CBIND(sqlite3_bind_text(module_id_statement, 1, mod_name.c_str(), mod_name.size(), SQLITE_STATIC));
  if (SQLITE_ROW != (last_retcode = sqlite3_step(module_id_statement))) {
    return -1;
  }
  int module_id = sqlite3_column_int(module_id_statement, 0);
  CFINALIZE(sqlite3_finalize(module_id_statement));
  return module_id;
}

int DBConn::getFunctionID(const string& f_name) {
  static string function_id_query = "SELECT ID FROM FUNCTION WHERE FUNCTION_IDENTIFIER=?;";
  sqlite3_stmt* function_id_statement;
  CPREPARE(sqlite3_prepare_v2(db, function_id_query.c_str(), -1, &function_id_statement, NULL));
  CBIND(sqlite3_bind_text(function_id_statement, 1, f_name.c_str(), f_name.size(), SQLITE_STATIC));
  if (SQLITE_ROW != (last_retcode = sqlite3_step(function_id_statement))) {
    return -1;
  }
  int function_id = sqlite3_column_int(function_id_statement, 0);
  CFINALIZE(sqlite3_finalize(function_id_statement));
  return function_id;
}

int DBConn::getGlobalVariableID(const string& g_name) {
  static string global_id_query = "SELECT ID FROM GLOBAL_VARIABLE WHERE GLOBAL_VARIABLE_IDENTIFIER=?;";
  sqlite3_stmt* global_id_statement;
  CPREPARE(sqlite3_prepare_v2(db, global_id_query.c_str(), -1, &global_id_statement, NULL));
  CBIND(sqlite3_bind_text(global_id_statement, 1, g_name.c_str(), g_name.size(), SQLITE_STATIC));
  if (SQLITE_ROW != (last_retcode = sqlite3_step(global_id_statement))) {
    return -1;
  }
  int global_id = sqlite3_column_int(global_id_statement, 0);
  CFINALIZE(sqlite3_finalize(global_id_statement));
  return global_id;
}

int DBConn::getTypeID(const string& t_name) {
  static string type_id_query = "SELECT ID FROM TYPE WHERE TYPE_IDENTIFIER=?;";
  sqlite3_stmt* type_id_statement;
  CPREPARE(sqlite3_prepare_v2(db, type_id_query.c_str(), -1, &type_id_statement, NULL));
  CBIND(sqlite3_bind_text(type_id_statement, 1, t_name.c_str(), t_name.size(), SQLITE_STATIC));
  if (SQLITE_ROW != (last_retcode = sqlite3_step(type_id_statement))) {
    return -1;
  }
  int type_id = sqlite3_column_int(type_id_statement, 0);
  CFINALIZE(sqlite3_finalize(type_id_statement));
  return type_id;
}

DBConn& DBConn::getInstance() {
  static DBConn instance;
  return instance;
}

// int DBConn::resultSetCallBack(void* data, int argc, char** argv,
//                               char** azColName) {
//   ResultSet* rs = (ResultSet*)data;
//   rs->rows++;
//   if (rs->rows == 1) {
//     rs->header.resize(argc);
//     for (int i = 0; i < argc; ++i) {
//       rs->header.push_back(azColName[i]);
//     }
//   }
//   vector<string> row(argc);
//   for (int i = 0; i < argc; ++i) {
//     row.push_back((argv[i] ? argv[i] : "NULL"));
//   }
//   rs->data.push_back(row);
//   return 0;
// }

void DBConn::execute(const string& query) {
   if ((last_retcode = sqlite3_exec(db, query.c_str(), NULL, NULL,
                                    &error_msg)) != SQLITE_OK) {
     cerr << "could not execute sqlite3 query:\n" + query + "\n";
     cerr << getLastErrorMsg() << endl;
     HEREANDNOW;
     sqlite3_free(error_msg);
   }
 }

string DBConn::getDBName() { return dbname; }

int DBConn::getLastRetCode() { return last_retcode; }

string DBConn::getLastErrorMsg() {
  return (error_msg != 0) ? string(error_msg) : "none";
}

void DBConn::DBInitializationAction() {
  // table storing the IR modules
  const static string ir_table_init_command =
      "CREATE TABLE IF NOT EXISTS IR_MODULE("
      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
      "MODULE_IDENTIFIER VARCHAR(255),"
      "SRC_HASH VARCHAR(255),"
      "IR_HASH VARCHAR(255),"
      "IR_CODE BLOB);";
  execute(ir_table_init_command);
  // table storing the functions
  const static string function_table_init_command =
      "CREATE TABLE IF NOT EXISTS FUNCTION("
      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
      "FUNCTION_IDENTIFIER VARCHAR(255),"
      "IS_VIRTUAL INTEGER);";
  execute(function_table_init_command);
  // table for storing the types
  const static string type_table_init_command =
      "CREATE TABLE IF NOT EXISTS TYPE("
      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
      "TYPE_IDENTIFIER VARCHAR(255));";
  execute(type_table_init_command);
  // table for storing the global variables
  const static string global_variable_init_command =
  		"CREATE TABLE IF NOT EXISTS GLOBAL_VARIABLE("
  		"ID INTEGER PRIMARY KEY AUTOINCREMENT,"
  		"GLOBAL_VARIABLE_IDENTIFIER VARCHAR(255));";
  execute(global_variable_init_command);
  // relation tables are defined here
  const static string ir_module_defines_function =
  		"CREATE TABLE IF NOT EXISTS IR_MODULE_DEFINES_FUNCTION("
  		"ID INTEGER PRIMARY KEY AUTOINCREMENT,"
  		"IR_MODULE_ID INTEGER,"
  		"FUNCTION_ID INTEGER);";
  execute(ir_module_defines_function);
  const static string ir_module_defines_global =
  		"CREATE TABLE IF NOT EXISTS IR_MODULE_DEFINES_GLOBAL_VARIABLE("
  		"ID INTEGER PRIMARY KEY AUTOINCREMENT,"
  		"IR_MODULE_ID INTEGER,"
  		"GLOBAL_VARIABLE_ID INTEGER);";
  execute(ir_module_defines_global);
  const static string type_has_vfunction_init_command =
      "CREATE TABLE IF NOT EXISTS TYPE_HAS_VFUNCTION("
  		"ID INTEGER PRIMARY KEY AUTOINCREMENT,"
      "TYPE_ID INTEGER,"
      "FUNCTION_ID INTEGER,"
      "VTABLE_INDEX INTEGER);";
  execute(type_has_vfunction_init_command);
}

// API for querying the IR Modules that correspond to the project under analysis
bool DBConn::containsIREntry(const string& mod_name) {
  static string query = "SELECT EXISTS(SELECT 1 FROM IR_MODULE WHERE MODULE_IDENTIFIER=? LIMIT 1);";
  sqlite3_stmt* stmt;
  CPREPARE(sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL));
  CBIND(sqlite3_bind_text(stmt, 1, mod_name.c_str(), mod_name.size(), NULL));
  CSTEP(sqlite3_step(stmt));
  char contains = sqlite3_column_int(stmt, 0);
  CFINALIZE(sqlite3_finalize(stmt));
  return contains;
}

bool DBConn::insertIRModule(const llvm::Module* module) {
  // compute the hash value from the original C/C++ source file
  string src_file_name(module->getModuleIdentifier());
  string src_buffer = readFile(src_file_name);
  string src_hash = to_string(hash<string>()(src_buffer));
  cout << "SRC_HASH: " << src_hash << endl;
  // misuse string as a smart buffer
  string ir_buffer;
  llvm::raw_string_ostream rso(ir_buffer);
  llvm::WriteBitcodeToFile(module, rso);
  rso.flush();
  // testing the ir buffer
  // cout << ir_buffer << endl;
  // query to insert a new IR entry
  static string query =
      "INSERT INTO IR_MODULE (MODULE_IDENTIFIER,"
      "SRC_HASH,"
      "IR_HASH,"
      "IR_CODE) "
      "VALUES (?,?,?,?);";
  // bind values to the prepared statement
  sqlite3_stmt* statement;
  CPREPARE(sqlite3_prepare_v2(db, query.c_str(), -1, &statement, NULL));
  // caution: left-most value index starts with '1'
  // use SQLITE_TRANSIENT when SQLite should copy the string (e.g. when string
  // is destroyed before query is executed)
  // use SQLITE_STATIC when the strings lifetime exceeds the point in time where
  // the query is executed
  CBIND(sqlite3_bind_text(statement, 1, module->getModuleIdentifier().c_str(),
                        module->getModuleIdentifier().size(), SQLITE_STATIC));
  // compute hash of front-end IR
  string ir_hash = to_string(hash<string>()(ir_buffer));
  CBIND(sqlite3_bind_text(statement, 2, src_hash.c_str(), src_hash.size(), SQLITE_STATIC));
  CBIND(sqlite3_bind_text(statement, 3, ir_hash.c_str(), ir_hash.size(), SQLITE_STATIC));
  CBIND(sqlite3_bind_blob(statement, 4, ir_buffer.data(), ir_buffer.size(), SQLITE_TRANSIENT));
  // execute statement and check if it works out
  CSTEP(sqlite3_step(statement));
  CFINALIZE(sqlite3_finalize(statement));
  return false;
}

bool DBConn::insertFunctionModuleDefinition(const string& f_name, const string& mod_name) {
  // retrieve the ID from the module
  int module_id = getModuleID(mod_name);
  // insert the function name
  static string insert_function_query = "INSERT INTO FUNCTION (FUNCTION_IDENTIFIER, IS_VIRTUAL) VALUES(?, ?);";
  sqlite3_stmt* insert_function_statement;
  CPREPARE(sqlite3_prepare_v2(db, insert_function_query.c_str(), -1, &insert_function_statement, NULL));
  CBIND(sqlite3_bind_text(insert_function_statement, 1, f_name.c_str(), f_name.size(), SQLITE_STATIC));
  CBIND(sqlite3_bind_int(insert_function_statement, 2, 0));
  CSTEP(sqlite3_step(insert_function_statement));
  // retrieve back the functions id
  int function_id = getFunctionID(f_name);
 // insert the relation
  static string insert_relation_query = "INSERT INTO IR_MODULE_DEFINES_FUNCTION (IR_MODULE_ID, FUNCTION_ID) VALUES(?, ?);";
  sqlite3_stmt* insert_relation_statement;
  CPREPARE(sqlite3_prepare_v2(db, insert_relation_query.c_str(), -1, &insert_relation_statement, NULL));
  CBIND(sqlite3_bind_int(insert_relation_statement, 1, module_id));
  CBIND(sqlite3_bind_int(insert_relation_statement, 2, function_id));
  CSTEP(sqlite3_step(insert_relation_statement));
  // free statements
  CFINALIZE(sqlite3_finalize(insert_function_statement));
  CFINALIZE(sqlite3_finalize(insert_relation_statement));
  return false;
}

bool DBConn::insertGlobalVariableModuleDefinition(const string& g_name, const string& mod_name) {
  // retrieve the ID from the module
  int module_id = getModuleID(mod_name);
  // insert the global name
  static string insert_global_query = "INSERT INTO GLOBAL_VARIABLE (GLOBAL_VARIABLE_IDENTIFIER) VALUES(?);";
  sqlite3_stmt* insert_global_statement;
  CPREPARE(sqlite3_prepare_v2(db, insert_global_query.c_str(), -1, &insert_global_statement, NULL));
  CBIND(sqlite3_bind_text(insert_global_statement, 1, g_name.c_str(), g_name.size(), SQLITE_STATIC));
  CSTEP(sqlite3_step(insert_global_statement));
  // retrieve back the functions id
  int global_id = getGlobalVariableID(g_name);
 // insert the relation
  static string insert_relation_query = "INSERT INTO IR_MODULE_DEFINES_GLOBAL_VARIABLE (IR_MODULE_ID, GLOBAL_VARIABLE_ID) VALUES(?, ?);";
  sqlite3_stmt* insert_relation_statement;
  CPREPARE(sqlite3_prepare_v2(db, insert_relation_query.c_str(), -1, &insert_relation_statement, NULL));
  CBIND(sqlite3_bind_int(insert_relation_statement, 1, module_id));
  CBIND(sqlite3_bind_int(insert_relation_statement, 2, global_id));
  CSTEP(sqlite3_step(insert_relation_statement));
  // free statements
  CFINALIZE(sqlite3_finalize(insert_global_statement));
  CFINALIZE(sqlite3_finalize(insert_relation_statement));
	return false;
}

unique_ptr<llvm::Module> DBConn::getIRModule(const string& module_id, llvm::LLVMContext& Context) {
  string ir_buffer;  // misue string as a smart buffer
  sqlite3_stmt* statement;
  static string query = "SELECT IR_CODE FROM IR_MODULE WHERE MODULE_IDENTIFIER=?;";
  CPREPARE(sqlite3_prepare_v2(db, query.c_str(), -1, &statement, NULL));
  CBIND(sqlite3_bind_text(statement, 1, module_id.c_str(), module_id.size(), SQLITE_STATIC));
  if (SQLITE_ROW != (last_retcode = sqlite3_step(statement))) {
    cout << "DB error: something went wrong in retrieving IR module" << endl;
    HEREANDNOW;
  }
  size_t nbytes = sqlite3_column_bytes(statement, 0);
  ir_buffer.resize(nbytes);
  memcpy(const_cast<char*>(ir_buffer.data()), sqlite3_column_blob(statement, 0),
         nbytes);
  CFINALIZE(sqlite3_finalize(statement));
  // parse the freshly retrieved byte sequence into an llvm::Module
  llvm::SMDiagnostic ErrorDiagnostics;
  unique_ptr<llvm::MemoryBuffer> MemBuffer = llvm::MemoryBuffer::getMemBuffer(ir_buffer);
  unique_ptr<llvm::Module> Mod = llvm::parseIR(*MemBuffer, ErrorDiagnostics, Context);
  // restore module identifier
  Mod->setModuleIdentifier(module_id);
  // check if everything has worked-out
  bool broken_debug_info = false;
  if (llvm::verifyModule(*Mod, &llvm::errs(), &broken_debug_info)) {
    cout << "verifying module failed!" << endl;
  }
  if (broken_debug_info) {
    cout << "debug info is broken!" << endl;
  }
  return Mod;
}

string DBConn::getModuleFunctionDefinition(const string& f_name) {
// retrieve module id
  static string module_id_query =
      "SELECT MODULE_ID FROM FUNCTIONS WHERE FUNCTION_IDENTIFIER=?;";
  sqlite3_stmt* mod_id_statement;
  CPREPARE(sqlite3_prepare_v2(db, module_id_query.c_str(), -1, &mod_id_statement, NULL));
  CBIND(sqlite3_bind_text(mod_id_statement, 1, f_name.c_str(), f_name.size(), SQLITE_STATIC));
  if (SQLITE_ROW != (last_retcode = sqlite3_step(mod_id_statement))) {
    cout << "DB error: something went wrong in retrieving MODULE_IDENTIFIER"
         << endl;
         HEREANDNOW;
  }
  int module_id = sqlite3_column_int(mod_id_statement, 0);
// retrieve module name
  static string module_name_query = "SELECT MODULE_IDENTIFIER FROM IR WHERE ID=?;";
  sqlite3_stmt* module_name_statement;
  CPREPARE(sqlite3_prepare_v2(db, module_name_query.c_str(), -1, &module_name_statement, NULL));
  CBIND(sqlite3_bind_int(module_name_statement, 1, module_id));
  if (SQLITE_ROW != (last_retcode = sqlite3_step(module_name_statement))) {
    cout << "DB error: something went wrong when retrieving MODULE_IDENTIFIER" << endl;
    HEREANDNOW;
  }
  string module_name((char*) sqlite3_column_text(module_name_statement, 0));
  CFINALIZE(sqlite3_finalize(mod_id_statement));
  CFINALIZE(sqlite3_finalize(module_name_statement));
  return module_name; 
}

void operator<<(DBConn& db, const ProjectIRCompiledDB& irdb) {
  for (auto& entry : irdb.modules) {
    db.insertIRModule(entry.second.get());
  }
  for (auto& entry : irdb.functions) {
    db.insertFunctionModuleDefinition(entry.first, entry.second);
  }
  for (auto& entry : irdb.globals) {
  	db.insertGlobalVariableModuleDefinition(entry.first, entry.second);
  }
}

void operator>>(DBConn& db, const ProjectIRCompiledDB& irdb) {}

size_t DBConn::getSRCHash(const string& mod_name) {
  static string src_hash_query = "SELECT SRC_HASH FROM IR WHERE MODULE_IDENTIFIER=?;";
  sqlite3_stmt* src_hash_stmt;
  CPREPARE(sqlite3_prepare_v2(db, src_hash_query.c_str(), src_hash_query.size(), &src_hash_stmt, NULL));
  CBIND(sqlite3_bind_text(src_hash_stmt, 1, mod_name.c_str(), mod_name.size(), SQLITE_STATIC));
  CSTEP(sqlite3_step(src_hash_stmt));
  size_t hash_val = stoull(string((char*) sqlite3_column_text(src_hash_stmt, 0)));
  CFINALIZE(sqlite3_finalize(src_hash_stmt));
  return hash_val;
}

bool DBConn::insertType(const string& type_name, VTable vtbl) {
  static string insert_type_query = "INSERT INTO TYPE (TYPE_IDENTIFIER) VALUES (?);";
  static string insert_relation_query = "INSERT INTO TYPE_HAS_VFUNCTION (TYPE_ID, FUNCTION_ID, VTABLE_INDEX) VALUES(?,?,?);";
  sqlite3_stmt* insert_type_stmt;
  sqlite3_stmt* insert_relation_stmt;
  // insert the type
  CPREPARE(sqlite3_prepare_v2(db, insert_type_query.c_str(), insert_type_query.size(), &insert_type_stmt, NULL));
  CBIND(sqlite3_bind_text(insert_type_stmt, 1, type_name.c_str(), type_name.size(), SQLITE_STATIC));
  CSTEP(sqlite3_step(insert_type_stmt));
  int type_id = getTypeID(type_name);
  for (unsigned idx = 0; idx < vtbl.getVTable().size(); ++idx) {
    // get the function id from the virtual function
    int function_id = getFunctionID(vtbl.getFunctionByIdx(idx));
    CPREPARE(sqlite3_prepare_v2(db, insert_relation_query.c_str(), insert_relation_query.size(), &insert_relation_stmt, NULL));
    CBIND(sqlite3_bind_int(insert_relation_stmt, 1, type_id));
    CBIND(sqlite3_bind_int(insert_relation_stmt, 2, function_id));
    CBIND(sqlite3_bind_int(insert_relation_stmt, 3, idx));
    CSTEP(sqlite3_step(insert_relation_stmt));
    CFINALIZE(sqlite3_finalize(insert_relation_stmt));
  }
  CFINALIZE(sqlite3_finalize(insert_type_stmt));
  return false;
}

VTable DBConn::getVTable(const string& type_name) {
  return VTable();
}

set<string> DBConn::getAllTypeIdentifiers() {
  set<string> types;
  static string get_types_query = "SELECT TYPE_IDENTIFIER FROM TYPES;";
  sqlite3_stmt* get_types_stmt;
  CPREPARE(sqlite3_prepare_v2(db, get_types_query.c_str(), get_types_query.size(), &get_types_stmt, NULL));
  while (SQLITE_ROW == sqlite3_step(get_types_stmt)) {
    types.insert(string((char*) sqlite3_column_text(get_types_stmt, 0)));
  }
  CFINALIZE(sqlite3_finalize(get_types_stmt));
  return types;
}

// bool insertLLVMStructHierarchyGraph(LLVMStructTypeHierarchy::digraph_t g) {
//   typename boost::graph_traits<LLVMStructTypeHierarchy::digraph_t>::edge_iterator ei_start, e_end;
// 	for (tie(ei_start, e_end) = boost::edges(g); ei_start != e_end; ++ei_start) {
// 		auto source = boost::source(*ei_start, g);
// 		auto target = boost::target(*ei_start, g);
// 		cout << g[source].name << " --> " << g[target].name << "\n";
// 	}
//   return false;
// }

void operator<<(DBConn& db, const LLVMStructTypeHierarchy& STH) {

   for (auto& entry : STH.vtable_map) {
       db.insertType(entry.first, entry.second);
   }
//   insertLLVMStructHierarchyGraph(STH.g);
}

void operator>>(DBConn& db, const LLVMStructTypeHierarchy& STH) {
  cout << "READ STH FROM DB\n";
}

void operator<<(DBConn& db, const PointsToGraph& PTG) {
	UNRECOVERABLE_CXX_ERROR_COND(db.isSynchronized(), "DBConn not synchronized with an ProjectIRCompiledDB object!");
	static PHSStringConverter converter(*db.IRDB);
  typedef boost::graph_traits<PointsToGraph::graph_t>::vertex_iterator vertex_iterator_t;
  for (pair<vertex_iterator_t, vertex_iterator_t> vp = boost::vertices(PTG.ptg); vp.first != vp.second; ++vp.first) {
    PTG.ptg[*vp.first].value->dump();
    string hsstringrep = converter.PToHStoreStringRep(PTG.ptg[*vp.first].value);
    cout << "HSStringRep: " << hsstringrep << "\n";
    cout << "Re converted\n";
    converter.HStoreStringRepToP(hsstringrep)->dump();
    cout << "\n\n";
  }
	cout << "writing points-to graph into hexastore\n";
	cout << "writing points-to graph of function ";
  for (const auto& fname : PTG.merge_stack) {
    cout << fname << " ";
  }
  cout << "into hexastore\n";
//  hexastore::Hexastore h("points_to_graph_hexastore.db");
  typedef boost::graph_traits<PointsToGraph::graph_t>::vertex_iterator vertex_iterator_t;
  cout << "vertices:\n";
  for (pair<vertex_iterator_t, vertex_iterator_t> vp = boost::vertices(PTG.ptg); vp.first != vp.second; ++vp.first) {
    cout << "id: " << PTG.ptg[*vp.first].id
         << " ir_code: " << PTG.ptg[*vp.first].ir_code
         << " hex name: " << converter.PToHStoreStringRep(PTG.ptg[*vp.first].value)
         << "\n";

  }
  cout << "edges:\n";
  typename boost::graph_traits<PointsToGraph::graph_t>::edge_iterator ei_start, e_end;
  for (tie(ei_start, e_end) = boost::edges(PTG.ptg); ei_start != e_end; ++ei_start) {
    auto source = boost::source(*ei_start, PTG.ptg);
    auto target = boost::target(*ei_start, PTG.ptg);
    cout << PTG.ptg[source].ir_code << " --> " << PTG.ptg[target].ir_code << "\n";
  }
  cout << "\n";
  cout << "\n";
  PTG.print();
  cout << "\n\n\n";
}

void operator>>(DBConn& db, const PointsToGraph& PTG) {
	UNRECOVERABLE_CXX_ERROR_COND(db.isSynchronized(), "DBConn not synchronized with an ProjectIRCompiledDB object!");
	static PHSStringConverter converter(*db.IRDB);
	cout << "reading points-to graph from hexastore\n";
}

size_t DBConn::getIRHash(const string& mod_name) {
  static string ir_hash_query = "SELECT IR_HASH FROM IR WHERE MODULE_IDENTIFIER=?;";
  sqlite3_stmt* ir_hash_stmt;
  CPREPARE(sqlite3_prepare_v2(db, ir_hash_query.c_str(), ir_hash_query.size(), &ir_hash_stmt, NULL));
  CBIND(sqlite3_bind_text(ir_hash_stmt, 1, mod_name.c_str(), mod_name.size(), SQLITE_STATIC));
  CSTEP(sqlite3_step(ir_hash_stmt));
  size_t hash_val = stoull(string((char*) sqlite3_column_text(ir_hash_stmt, 0)));
  CFINALIZE(sqlite3_finalize(ir_hash_stmt));
  return hash_val;
}

set<string> DBConn::getAllModuleIdentifiers() { 
  set<string> module_names;
  static string mod_names_query = "SELECT MODULE_IDENTIFIER FROM IR_MODULE;";
  sqlite3_stmt* mod_names_stmt;
  CPREPARE(sqlite3_prepare_v2(db, mod_names_query.c_str(), mod_names_query.size(), &mod_names_stmt, NULL));
  while (SQLITE_ROW == sqlite3_step(mod_names_stmt)) {
    module_names.insert(string((char*) sqlite3_column_text(mod_names_stmt, 0)));
  }
  CFINALIZE(sqlite3_finalize(mod_names_stmt));
  return module_names;
}
