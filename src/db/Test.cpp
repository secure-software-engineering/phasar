#include <iostream>
#include <memory>
#include "Hexastore.hh"

using namespace std;

int main() {

  Hexastore h("test.db");

  h.put((string[]){"mary", "likes", "hexastore"});

  return 0;
}
