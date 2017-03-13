#include <iostream>
#include <memory>
#include "Hexastore.hh"

using namespace std;

int main() {

  Hexastore h("test.sqlite");

  h.put((string[]){"mary", "likes", "hexastores"});
  h.put((string[]){"mary", "likes", "apples"});
  h.put((string[]){"peter", "likes", "apples"});
  h.put((string[]){"peter", "hates", "hexastores"});


  return 0;
}
