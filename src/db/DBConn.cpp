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
    cerr << "SQLITE code: " << last_retcode << endl;
  }
  DBInitializationAction();
}

DBConn::~DBConn() {
  if ((last_retcode = sqlite3_close(db)) != SQLITE_OK) {
    cerr << "could not close sqlite3 database properly!" << endl;
    cerr << "SQLITE code: " << last_retcode << endl;
  }
}

DBConn& DBConn::getInstance() {
  static DBConn instance;
  return instance;
}

int DBConn::resultSetCallBack(void* data, int argc, char** argv,
                              char** azColName) {
  ResultSet* rs = (ResultSet*)data;
  rs->rows++;
  if (rs->rows == 1) {
    rs->header.resize(argc);
    for (int i = 0; i < argc; ++i) {
      rs->header.push_back(azColName[i]);
    }
  }
  vector<string> row(argc);
  for (int i = 0; i < argc; ++i) {
    row.push_back((argv[i] ? argv[i] : "NULL"));
  }
  rs->data.push_back(row);
  return 0;
}

ResultSet DBConn::execute(const string& query) {
  ResultSet result;
  if ((last_retcode = sqlite3_exec(db, query.c_str(), DBConn::resultSetCallBack,
                                   (void*)&result, &error_msg)) != SQLITE_OK) {
    cerr << "could not execute sqlite3 query!" << endl;
    cerr << getLastErrorMsg() << endl;
    sqlite3_free(error_msg);
  }
  return result;
}

string DBConn::getDBName() { return dbname; }

int DBConn::getLastRetCode() { return last_retcode; }

string DBConn::getLastErrorMsg() {
  return (error_msg != 0) ? string(error_msg) : "none";
}

void DBConn::createDBSchemeForAnalysis(const string& analysis_name) {
  const static string analysis_initialization_command = "";
  execute(analysis_initialization_command);
}

void DBConn::DBInitializationAction() {
  const static string database_initialization_command =
      "CREATE TABLE IF NOT EXISTS IR("
      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
      "MODULE_ID VARCHAR(255),"
      "SRC_HASH VARCHAR(255),"
      "FRONTEND_IR_HASH VARCHAR(255),"
      "FRONTEND_IR BLOB,"
      "OPTIMIZED_IR BLOB);";
  execute(database_initialization_command);
}

vector<string> DBConn::getAllTables() {
  const static string list_tables =
      "SELECT name FROM sqlite_master WHERE type='table';";
  ResultSet rs = execute(list_tables);
  vector<string> tables;
  for (auto row : rs.data) {
    for (auto entry : row) {
      tables.push_back(entry);
    }
  }
  return tables;
}

bool DBConn::tableExists(const string& table_name) {
  vector<string> tables = getAllTables();
  for (auto table : tables) {
    if (table == table_name) {
      return true;
    }
  }
  return false;
}

bool DBConn::dropTable(const string& table_name) {
  if (!tableExists(table_name)) return false;
  const static string drop_table = "DROP TABLE " + table_name + ";";
  execute(drop_table);
  return true;
}

// API for querying the IR Modules that correspond to the project under analysis
bool DBConn::containsIREntry(string mod_name) {
  static string query =
      "SELECT EXISTS(SELECT 1 FROM IR WHERE NAME='" + mod_name + "' LIMIT 1);";
  ResultSet result = execute(query);
  return result.data[0][0] == "1";
}

bool DBConn::insertIR(const llvm::Module* module) {
  // misuse string as a smart buffer
  string ir_buffer;
  llvm::raw_string_ostream rso(ir_buffer);
  llvm::WriteBitcodeToFile(module, rso);
  rso.flush();
  cout << ir_buffer << endl;
  // query to insert a new IR entry
  static string query =
      "INSERT INTO IR (MODULE_ID,"
      "FRONTEND_IR_HASH,"
      "FRONTEND_IR) "
      "VALUES (?,?,?);";
  // bind values to the prepared statement
  sqlite3_stmt* statement;
  last_retcode = sqlite3_prepare_v2(db, query.c_str(), -1, &statement, NULL);
  // caution: left-most value index starts with '1'
  // use SQLITE_TRANSIENT when SQLite should copy the string (e.g. when string
  // is destroyed before query is executed)
  // use SQLITE_STATIC when the strings lifetime exceeds the point in time where
  // the query is executed
  last_retcode =
      sqlite3_bind_text(statement, 1, module->getModuleIdentifier().c_str(),
                        module->getModuleIdentifier().size(), SQLITE_STATIC);
  // compute hash of front-end IR
  string ir_hash = to_string(hash<string>()(ir_buffer));
  last_retcode = sqlite3_bind_text(statement, 2, ir_hash.c_str(),
                                   ir_hash.size(), SQLITE_STATIC);
  last_retcode = sqlite3_bind_blob(statement, 3, ir_buffer.data(),
                                   ir_buffer.size(), SQLITE_TRANSIENT);
  if (last_retcode != SQLITE_OK) {
    cout << "DB error: something went wrong in binding" << endl;
  }
  // execute statement and check if it works out
  if (SQLITE_DONE != (last_retcode = sqlite3_step(statement))) {
    cout << "DB error: something went wrong in inserting IR module" << endl;
  }
  sqlite3_finalize(statement);
  return false;
}

unique_ptr<llvm::Module> DBConn::getIR(string module_id,
                                       llvm::LLVMContext& Context) {
  string ir_buffer;  // misue string as a smart buffer
                     //	string module_id;
  sqlite3_stmt* statement;
  static string query = "SELECT FRONTEND_IR FROM IR WHERE MODULE_ID=?;";
  last_retcode = sqlite3_prepare_v2(db, query.c_str(), -1, &statement, NULL);
  last_retcode = sqlite3_bind_text(statement, 1, module_id.c_str(),
                                   module_id.size(), SQLITE_STATIC);
  if (SQLITE_ROW != (last_retcode = sqlite3_step(statement))) {
    cout << "DB error: something went wrong in retrieving IR module" << endl;
  }
  size_t nbytes = sqlite3_column_bytes(statement, 0);
  ir_buffer.resize(nbytes);
  memcpy(const_cast<char*>(ir_buffer.data()), sqlite3_column_blob(statement, 0),
         nbytes);
  last_retcode = sqlite3_finalize(statement);
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

void operator<<(DBConn& db, const ProjectIRCompiledDB& irdb) {}

size_t getSourceHash(string mod_name) { return 0; }

size_t getIRHash(string mod_name) { return 0; }


set<string> getAllIRModuleNames() { return {}; }