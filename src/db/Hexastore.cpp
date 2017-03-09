#include "./Hexastore.hh"

#include <sqlite3.h>
#include <sstream>
#include <fstream>
#include <iostream>

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
  in.open("../db/hs_init.sql");

  const char* query = static_cast<std::ostringstream&>(ostringstream{} << in.rdbuf()).str().c_str();

  char* err;

  cout << "Hallo" << query;

  sqlite3_exec(this->db, query, callback, 0, &err);

}

Hexastore::~Hexastore() {
  sqlite3_close(this->db);
}

void Hexastore::put(string[3]) {
  
}

map<string, string> Hexastore::query(vector<string>) {

  return map<string, string>();
}
