#include <iostream>
#include <memory>
#include <assert.h>
#include "Hexastore.hh"


using namespace std;
using namespace hexastore;

int main() {

  hexastore::Hexastore h("test.sqlite");

  h.put((string[]){"mary", "likes", "hexastores"});
  h.put((string[]){"mary", "likes", "apples"});
  h.put((string[]){"peter", "likes", "apples"});
  h.put((string[]){"peter", "hates", "hexastores"});

  auto output = [](hs_result res) {
    cout << res.subject << " " << res.predicate << " " << res.object << "\n";
    return 0;
  };

  h.get((vector<string>){"?", "likes", "?"}, output);
  h.get((vector<string>){"peter", "hates", "?"}, output);



  return 0;
}
