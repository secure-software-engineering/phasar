#include <vector>
#include <string>
#include <map>
#include <sqlite3.h>

using namespace std;

class Hexastore {

private:
  void prepare();
  sqlite3* db;

public:
  Hexastore(string filename);
  ~Hexastore();
  void put(string[3]);
  map<string, string> query(vector<string>);
};
