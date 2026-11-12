#include <dirent.h>
#include <string.h>

static int keep(const struct dirent *D) { return D->d_name[0] != '.'; }

static int byName(const struct dirent **A, const struct dirent **B) {
  return strcmp((*A)->d_name, (*B)->d_name);
}

int main(void) {
  struct dirent **List = 0;
  return scandir(".", &List, keep, byName);
}
