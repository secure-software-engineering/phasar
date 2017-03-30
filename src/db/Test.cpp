#include <iostream>
#include <array>
#include <algorithm>
#include <assert.h>
#include "Hexastore.hh"

using namespace std;
using namespace hexastore;

int hs_test_main() {
  hexastore::Hexastore h("test.sqlite");
  // init with some stuff
  h.put({"mary", "likes", "hexastores"});
  h.put({"mary", "likes", "apples"});
  h.put({"peter", "likes", "apples"});
  h.put({"peter", "hates", "hexastores"});
  h.put({"frank", "admires", "bananas"});
  //query some stuff
  cout << "Who likes what?" << "\n";
  auto result = h.get({"?", "likes", "?"});
  for_each(result.begin(), result.end(), [](hs_result r){
	  cout << r << endl;
  });
  cout << "\n";
  cout << "What does peter hate?" << "\n";
  result = h.get({"peter", "hates", "?"});
  for_each(result.begin(), result.end(), [](hs_result r){
	  cout << r << endl;
  });
  cout << "\n";
  cout << "Who admires something?" << "\n";
  result = h.get({"?", "admires", "?"});
  for_each(result.begin(), result.end(), [](hs_result r){
	  cout << r << endl;
  });
  return 0;
}
