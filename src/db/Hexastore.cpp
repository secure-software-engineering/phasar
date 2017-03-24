#include "./Hexastore.hh"
#include "./Queries.hh"

#include <sqlite3.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>

using namespace boost;
using namespace hexastore;

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
  


  const string query = hexastore::INIT;

  char* err;


  sqlite3_exec(this->db, query.c_str(), callback, 0, &err);

  if (err != NULL)
    cout << err << "\n\n";


}

Hexastore::~Hexastore() {
  sqlite3_close(this->db);
}

void Hexastore::put(string* values) {
  this->doPut(hexastore::SPO_INSERT, values);
  this->doPut(hexastore::SOP_INSERT, values);
  this->doPut(hexastore::PSO_INSERT, values);
  this->doPut(hexastore::POS_INSERT, values);
  this->doPut(hexastore::OSP_INSERT, values);
  this->doPut(hexastore::OPS_INSERT, values);
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

vector<hs_result> Hexastore::get(vector<string> query, function<int(hs_result)> cb) {
  vector<hs_result> result;

  
  if (query.size() != 3) {
    return vector<hs_result>();
  }

  string querystring;

  if (query[0] == "?") {
    if (query[1] == "?") {
      if (query[2] == "?") {
        querystring = SEARCH_XXX;
      } else {
        querystring = SEARCH_XXO;
      }
    } else {
      if (query[2] == "?") {
        querystring = SEARCH_XPX;
      } else {
        querystring = SEARCH_XPO;
      }
    }
  } else {
    if (query[1] == "?") {
      if (query[2] == "?") {
        querystring = SEARCH_SXX;
      } else {
        querystring = SEARCH_SXO;
      }
    } else {
      if (query[2] == "?") {
        querystring = SEARCH_SPX;
      } else {
        querystring = SEARCH_SPO;
      }
    }
  }

  string compiled_query = str(format(querystring) % query[0] % query[1] % query[2]);

  char* err;

  auto sqlite_cb = [] (void* cb, int argc, char **argv, char **azColName) {
    hs_result r;
    r.subject = argv[0];
    r.predicate = argv[1];
    r.object = argv[2];
    function<int(hs_result)> cb_function = *((function<int(hs_result)>*) cb);
    cb_function(r);
    // cout << argv[0] << " " << argv[1] << " " << argv[2] << "\n";
    return 0;
  };
  sqlite3_exec(this->db, compiled_query.c_str(), sqlite_cb, &cb, &err);


  if(err != NULL) {
    cout << err;
  }

  return result;

}
