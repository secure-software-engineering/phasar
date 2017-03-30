#ifndef HEXASTORE_HH_
#define HEXASTORE_HH_

#include <iostream>
#include <array>
#include <vector>
#include <string>
#include <sqlite3.h>
#include <iostream>
#include "Queries.hh"
#include <boost/format.hpp>

using namespace std;

namespace hexastore
{

struct hs_result {
    string subject;
    string predicate;
    string object;
    hs_result() = default;
    hs_result(string s, string p, string o) : subject(s), predicate(p), object(o) {}
    friend ostream& operator<< (ostream& os, const hs_result& hsr) {
    	return os << "[ subject: " << hsr.subject << " | predicate: " << hsr.predicate << " | object: " << hsr.object << " ]";
    }
};

class Hexastore {
  private:
    sqlite3* db;
    static int callback(void *NotUsed, int argc, char **argv, char **azColName);
    void doPut(string query, array<string, 3> edge);

  public:
    Hexastore(string filename);
    ~Hexastore();
    void put(array<string, 3> edge);
    vector<hs_result> get(array<string, 3> edge_query, size_t result_size_hint=0);
};

} /* end namespace hexastore */

#endif /* HEXASTORE_HH_ */
