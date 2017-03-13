#include "./Hexastore.hh"
#include "./Queries.hh"

#include <sqlite3.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>

using namespace boost;

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}

Hexastore::Hexastore(string filename) {
  sqlite3_open(filename.c_str(), &this->db);
  

  ifstream in;
  in.open("../src/db/hs_init.sql");

  const string query = static_cast<std::ostringstream&>(ostringstream{} << in.rdbuf()).str();

  char* err;


  sqlite3_exec(this->db, query.c_str(), callback, 0, &err);

  if (err != NULL)
    cout << err << "\n";

}

Hexastore::~Hexastore() {
  sqlite3_close(this->db);
}

void Hexastore::put(string* values) {
  this->doPut(hs_queries::spo_insert, values);
  this->doPut(hs_queries::sop_insert, values);
  this->doPut(hs_queries::pso_insert, values);
  this->doPut(hs_queries::pos_insert, values);
  this->doPut(hs_queries::osp_insert, values);
  this->doPut(hs_queries::ops_insert, values);
}

map<string, string> Hexastore::query(vector<string>) {

  return map<string, string>();
}

void Hexastore::close() {
  sqlite3_close(this->db);
}

void Hexastore::doPut(string query, string* values) {
  string compiled_query = str(format(query) % values[0] % values[1] % values[2]);

  char* err;
  sqlite3_exec(this->db, compiled_query.c_str(), callback, 0, &err);

  if (err != NULL)
    cout << err;

}
