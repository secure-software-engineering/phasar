/*
 * DBConn.cpp
 *
 *  Created on: 23.08.2016
 *      Author: pdschbrt
 */

#include "DBConn.hh"

DBConn::DBConn(const string name) : dbname(name) {
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
     cerr << "could not execute sqlite3 query!\n";
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
      "CREATE TABLE IF NOT EXISTS IR("
      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
      "MODULE_IDENTIFIER VARCHAR(255),"
      "SRC_HASH VARCHAR(255),"
      "IR_HASH VARCHAR(255),"
      "IR BLOB);";
  execute(ir_table_init_command);
  // table storing the function to module mapping
  const static string function_table_init_command =
      "CREATE TABLE IF NOT EXISTS FUNCTIONS("
      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
      "FUNCTION_IDENTIFIER VARCHAR(255),"
      "MODULE_ID INTEGER);";
  execute(function_table_init_command);
  // tables for storing the vtables
  const static string type_table_init_command =
      "CREATE TABLE IF NOT EXISTS TYPES("
      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
      "TYPE_IDENTIFIER VARCHAR(255));";
  execute(type_table_init_command);
  const static string vfunction_table_init_command = 
      "CREATE TABLE IF NOT EXISTS VFUNCTIONS("
      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
      "FUNCTIONS_ID INTEGER,"
      "ENTRY INTEGER);";
  execute(vfunction_table_init_command);
  const static string type_has_vfunction_init_command =
      "CREATE TABLE IF NOT EXISTS TYPE_HAS_VFUNCTION("
      "TYPE_ID INTEGER,"
      "VFUNCTION_ID INTEGER);";
  execute(type_has_vfunction_init_command);
}

// API for querying the IR Modules that correspond to the project under analysis
bool DBConn::containsIREntry(string mod_name) {
  static string query = "SELECT EXISTS(SELECT 1 FROM IR WHERE MODULE_IDENTIFIER=? LIMIT 1);";
  sqlite3_stmt* stmt;
  CPREPARE(sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL));
  CBIND(sqlite3_bind_text(stmt, 1, mod_name.c_str(), mod_name.size(), NULL));
  CSTEP(sqlite3_step(stmt));
  char contains = sqlite3_column_int(stmt, 0);
  CFINALIZE(sqlite3_finalize(stmt));
  return contains;
}

bool DBConn::insertIR(const llvm::Module* module) {
  // compute the hash value from the original C/C++ source file
  ifstream src_file(module->getModuleIdentifier(), ios::binary);
  if (!src_file.is_open()) {
    cout << "DB error: could not open source file" << endl;
    HEREANDNOW;
  }
  src_file.seekg(0, src_file.end);
  size_t src_size = src_file.tellg();
  src_file.seekg(0, src_file.beg);
  string src_buffer;
  src_buffer.resize(src_size);
  src_file.read(const_cast<char*>(src_buffer.data()), src_size);
  src_file.close();
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
      "INSERT INTO IR (MODULE_IDENTIFIER,"
      "SRC_HASH,"
      "IR_HASH,"
      "IR) "
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
  CBIND(sqlite3_bind_text(statement, 2, src_hash.c_str(),
                                   src_hash.size(), SQLITE_STATIC));
  CBIND(sqlite3_bind_text(statement, 3, ir_hash.c_str(),
                                   ir_hash.size(), SQLITE_STATIC));
  CBIND(sqlite3_bind_blob(statement, 4, ir_buffer.data(),
                                   ir_buffer.size(), SQLITE_TRANSIENT));
  // execute statement and check if it works out
  CSTEP(sqlite3_step(statement));
  CFINALIZE(sqlite3_finalize(statement));
  return false;
}

unique_ptr<llvm::Module> DBConn::getIR(string module_id,
                                       llvm::LLVMContext& Context) {
  string ir_buffer;  // misue string as a smart buffer
                     //	string module_id;
  sqlite3_stmt* statement;
  static string query = "SELECT IR FROM IR WHERE MODULE_IDENTIFIER=?;";
  CPREPARE(sqlite3_prepare_v2(db, query.c_str(), -1, &statement, NULL));
  CBIND(sqlite3_bind_text(statement, 1, module_id.c_str(),
                                   module_id.size(), SQLITE_STATIC));
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
  unique_ptr<llvm::MemoryBuffer> MemBuffer =
      llvm::MemoryBuffer::getMemBuffer(ir_buffer);
  unique_ptr<llvm::Module> Mod =
      llvm::parseIR(*MemBuffer, ErrorDiagnostics, Context);
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

bool DBConn::insertFunctionModuleDefinition(string f_name, string mod_name) {
  // retrieve the ID from the module
  static string module_id_query =
      "SELECT ID FROM IR WHERE MODULE_IDENTIFIER=?;";
  sqlite3_stmt* mod_id_statement;
  CPREPARE(sqlite3_prepare_v2(db, module_id_query.c_str(), -1,
                                    &mod_id_statement, NULL));
  CBIND(sqlite3_bind_text(mod_id_statement, 1, mod_name.c_str(),
                                   mod_name.size(), SQLITE_STATIC));
  if (SQLITE_ROW != (last_retcode = sqlite3_step(mod_id_statement))) {
    cout << "DB error: something went wrong in retrieving MODULE_IDENTIFIER"
         << endl;
         HEREANDNOW;
  }
  int module_id = sqlite3_column_int(mod_id_statement, 0);
  // insert the function name
   static string insert_query = "INSERT INTO FUNCTIONS (FUNCTION_IDENTIFIER, MODULE_ID) VALUES(?, ?);";
   sqlite3_stmt* insert_statement;
   CPREPARE(sqlite3_prepare_v2(db, insert_query.c_str(), -1,
                                     &insert_statement, NULL));
   CBIND(sqlite3_bind_text(insert_statement, 1, f_name.c_str(),
                                    f_name.size(), SQLITE_STATIC));
   CBIND(sqlite3_bind_int(insert_statement, 2, module_id));
   CSTEP(sqlite3_step(insert_statement));
   CFINALIZE(sqlite3_finalize(mod_id_statement));
   CFINALIZE(sqlite3_finalize(insert_statement));
   return false;
}

string DBConn::getModuleFunctionDefinition(string f_name) { 
// retrieve module id
  static string module_id_query =
      "SELECT MODULE_ID FROM FUNCTIONS WHERE FUNCTION_IDENTIFIER=?;";
  sqlite3_stmt* mod_id_statement;
  CPREPARE(sqlite3_prepare_v2(db, module_id_query.c_str(), -1,
                                    &mod_id_statement, NULL));
  CBIND(sqlite3_bind_text(mod_id_statement, 1, f_name.c_str(),
                                   f_name.size(), SQLITE_STATIC));
  if (SQLITE_ROW != (last_retcode = sqlite3_step(mod_id_statement))) {
    cout << "DB error: something went wrong in retrieving MODULE_IDENTIFIER"
         << endl;
         HEREANDNOW;
  }
  int module_id = sqlite3_column_int(mod_id_statement, 0);
// retrieve module name
  static string module_name_query = "SELECT MODULE_IDENTIFIER FROM IR WHERE ID=?;";
  sqlite3_stmt* module_name_statement;
  CPREPARE(sqlite3_prepare_v2(db, module_name_query.c_str(), -1,
                                    &module_name_statement, NULL));
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
    db.insertIR(entry.second.get());
  }
  if (!irdb.functions.empty()) {
    for (auto& entry : irdb.functions) {
      db.insertFunctionModuleDefinition(entry.first, entry.second);
    }
  }
}

void operator>>(DBConn& db, const ProjectIRCompiledDB& irdb) {}

size_t DBConn::getSRCHash(string mod_name) { 
  static string src_hash_query = "SELECT SRC_HASH FROM IR WHERE MODULE_IDENTIFIER=?;";
  sqlite3_stmt* src_hash_stmt;
  CPREPARE(sqlite3_prepare_v2(db, src_hash_query.c_str(), src_hash_query.size(), &src_hash_stmt, NULL));
  CBIND(sqlite3_bind_text(src_hash_stmt, 1, mod_name.c_str(), mod_name.size(), SQLITE_STATIC));
  CSTEP(sqlite3_step(src_hash_stmt));
  size_t hash_val = stoull(string((char*) sqlite3_column_text(src_hash_stmt, 0)));
  CFINALIZE(sqlite3_finalize(src_hash_stmt));
  return hash_val;
}

bool DBConn::insertType(string type_name, VTable vtbl) {
  static string insert_type_query = "INSERT INTO TYPES (TYPE_IDENTIFIER) VALUES (?);";
  static string get_func_id_query = "SELECT ID FROM FUNCTIONS WHERE FUNCTION_IDENTIFIER=?;";
  static string insert_vfunc_query = "INSERT INTO VFUNCTIONS (FUNCTIONS_ID, ENTRY) VALUES (?,?);";
  static string get_type_id_query = "SELECT ID FROM TYPES WHERE TYPE_IDENTIFIER=?;";
  static string get_vfunc_id_query = "SELECT ID FROM VFUNCTIONS WHERE FUNCTIONS_ID=?;";
  static string insert_type_vfunc_query = "INSERT INTO TYPE_HAS_VFUNCTION (TYPE_ID, VFUNCTION_ID) VALUES(?,?);";
  sqlite3_stmt* insert_type_stmt;
  sqlite3_stmt* get_func_id_stmt;
  sqlite3_stmt* insert_vfunc_stmt;
  sqlite3_stmt* get_type_id_stmt;
  sqlite3_stmt* get_vfunc_id_stmt;
  sqlite3_stmt* insert_type_vfunc_stmt;
  // insert the type
  CPREPARE(sqlite3_prepare_v2(db, insert_type_query.c_str(), insert_type_query.size(), &insert_type_stmt, NULL));
  CBIND(sqlite3_bind_text(insert_type_stmt, 1, type_name.c_str(), type_name.size(), SQLITE_STATIC));
  CSTEP(sqlite3_step(insert_type_stmt));
  // get the type id
  CPREPARE(sqlite3_prepare_v2(db, get_type_id_query.c_str(), get_type_id_query.size(), &get_type_id_stmt, NULL));
  CBIND(sqlite3_bind_text(get_type_id_stmt, 1, type_name.c_str(), type_name.size(), SQLITE_STATIC));
  CSTEP(sqlite3_step(get_type_id_stmt));
  int type_id = sqlite3_column_int(get_type_id_stmt, 0);
  CFINALIZE(sqlite3_finalize(get_type_id_stmt));
  for (auto& vfunc : vtbl.getVTable()) {
    // get the function id from the virtual function
    cout << vfunc << endl;
    CPREPARE(sqlite3_prepare_v2(db, get_func_id_query.c_str(), get_func_id_query.size(), &get_func_id_stmt, NULL));
    CBIND(sqlite3_bind_text(get_func_id_stmt, 1, vfunc.c_str(), vfunc.size(), SQLITE_STATIC));
    CSTEP(sqlite3_step(get_func_id_stmt));
    int function_id = sqlite3_column_int(get_func_id_stmt, 0);
    CFINALIZE(sqlite3_finalize(get_func_id_stmt));
    cout << "FUNCTION ID: " << function_id << endl;
    // insert the virtual function into the vfunctions table
   int vtable_entry = vtbl.getEntryByFunctionName(vfunc);
   CPREPARE(sqlite3_prepare_v2(db, insert_vfunc_query.c_str(), insert_vfunc_query.size(), &insert_vfunc_stmt, NULL));
   CBIND(sqlite3_bind_int(insert_vfunc_stmt, 1, function_id));
   CBIND(sqlite3_bind_int(insert_vfunc_stmt, 2, vtable_entry));
   CSTEP(sqlite3_step(insert_vfunc_stmt));
   CFINALIZE(sqlite3_finalize(insert_vfunc_stmt));
   // get the vfunction id
   CPREPARE(sqlite3_prepare_v2(db, get_vfunc_id_query.c_str(), get_vfunc_id_query.size(), &get_vfunc_id_stmt, NULL));
   CBIND(sqlite3_bind_int(get_vfunc_id_stmt, 1, function_id));
   CSTEP(sqlite3_step(get_vfunc_id_stmt));
   int vfunc_id = sqlite3_column_int(get_vfunc_id_stmt, 0);
   CFINALIZE(sqlite3_finalize(get_vfunc_id_stmt));
   // insert type id and vfunction id into the relation
   CPREPARE(sqlite3_prepare_v2(db, insert_type_vfunc_query.c_str(), insert_type_vfunc_query.size(), &insert_type_vfunc_stmt, NULL));
   CBIND(sqlite3_bind_int(insert_type_vfunc_stmt, 1, type_id));
   CBIND(sqlite3_bind_int(insert_type_vfunc_stmt, 2, vfunc_id));
   CSTEP(sqlite3_step(insert_type_vfunc_stmt));
   CFINALIZE(sqlite3_finalize(insert_type_vfunc_stmt));
   }
  CFINALIZE(sqlite3_finalize(insert_type_stmt));
  return false;
}

VTable DBConn::getVTable(string type_name) {
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

// LLVMStructTypeHierarchy::digraph_t getLLVMStructTypeHierarchyGraph();

void operator<<(DBConn& db, const LLVMStructTypeHierarchy& STH) {
   cout << "WRITE STH TO DB\n";
   for (auto& entry : STH.vtable_map) {
       db.insertType(entry.first, entry.second);
   }
//   insertLLVMStructHierarchyGraph(STH.g);
}

void operator>>(DBConn& db, const LLVMStructTypeHierarchy& STH) {
  cout << "READ STH FROM DB\n";
}

size_t DBConn::getIRHash(string mod_name) { 
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
  static string mod_names_query = "SELECT MODULE_IDENTIFIER FROM IR;";
  sqlite3_stmt* mod_names_stmt;
  CPREPARE(sqlite3_prepare_v2(db, mod_names_query.c_str(), mod_names_query.size(), &mod_names_stmt, NULL));
  while (SQLITE_ROW == sqlite3_step(mod_names_stmt)) {
    module_names.insert(string((char*) sqlite3_column_text(mod_names_stmt, 0)));
  }
  CFINALIZE(sqlite3_finalize(mod_names_stmt));
  return module_names;
}
