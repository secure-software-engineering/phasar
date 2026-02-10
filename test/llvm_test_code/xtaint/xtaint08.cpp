#define PHASAR_ENABLE_TAINT_CONFIGURATION_API
#include "../../../devapis/taint/phasar_taint_config_api.h"

#include <cstdio>

struct Bar {
  int x;
  int y;
};

void foo(int x, Bar &y) { y.x = x; }

int main(int argc, char *argv[]) {
  PHASAR_DECLARE_VAR_AS_SOURCE(argc);
  PHASAR_DECLARE_COMPLETE_FUN_AS_SINK(printf);

  Bar x = {0, 0};
  foo(argc, x);

  printf("%d\n", x.x);
  printf("%d\n", x.y);
}
