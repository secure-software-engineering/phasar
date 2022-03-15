#include <string>

int main(int argc, char *argv[]) {
  int FeatureSelector = argc; // = 1
  switch (FeatureSelector) {
  case 0:
    return 1337;
  case 1:
    break;
  case 2:
    break;
  default:
    break;
  }
  return 0;
}
