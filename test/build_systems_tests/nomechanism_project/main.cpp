#include "source_1.h"
#include "source_2.h"
#include <iostream>
using namespace std;

int global = 10;

int multiply(int a, int b) { return a + b; }

int main() {
  int i = 3;
  int j = 5;
  int k = multiply(i, j);
  i = j * 10;
  j += 2;
  j = increment(j);
  k = add_and_div(k, i, j);
  k += global;
  cout << k << endl;
  return 0;
}
