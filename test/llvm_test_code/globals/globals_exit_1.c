#include <stdlib.h>

static int Count;

static void viaAtExit(void) { ++Count; }

static void viaOnExit(int Status, void *Arg) { *(int *)Arg += Status; }

static void viaQuickExit(void) { ++Count; }

int main(int Argc, char **Argv) {
  int *Local = malloc(sizeof(int));
  atexit(viaAtExit);
  on_exit(viaOnExit, Local);
  at_quick_exit(viaQuickExit);
  if (Argc > 1) {
    quick_exit(0);
  }
  return 0;
}
