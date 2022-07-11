#include "src2.h"

int main(int argc, char **argv) {
  int a = argc;
  // id() sei unbekannt
  // Alle dff die vor dem call gelten müssen nach dem call weitergetrackt werden
  // id() selbst kann durch den Aufruf weitere neue dffs generieren bzw. killen
  int b = id(a);
  int c = foo(b);
  return 0;
}
