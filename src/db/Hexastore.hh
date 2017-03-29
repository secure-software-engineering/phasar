#include <vector>
#include <string>
#include <map>
#include <sqlite3.h>
#include <functional>

using namespace std;

namespace hexastore {

  typedef struct {
    string subject;
    string predicate;
    string object;
  } hs_result;

  class Hexastore {

  private:
    void prepare();
    sqlite3* db;

  public:
    Hexastore(string filename);
    ~Hexastore();
    void put(string subject, string predicate, string object);
    vector<hs_result> get(vector<string> query, function<int (hs_result)> cb = [](hs_result){return 0;});
    void close();
  private:
    void doPut(string query, string subject, string predicate, string object);
  };


}
