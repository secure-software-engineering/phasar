#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <ftw.h>
#include <link.h>
#include <stdlib.h>
#include <threads.h>

static int thrdWorker(void *Arg) { return *(int *)Arg; }

static int visitObj(struct dl_phdr_info *Info, size_t Size, void *Data) {
  return Info->dlpi_phnum + (int)Size + *(int *)Data;
}

static int visitFile(const char *Path, const struct stat *Sb, int Flag,
                     struct FTW *Buf) {
  return Path[0] + Flag + Buf->level + (int)Sb->st_size;
}

int main(void) {
  static int State;
  thrd_t T;
  thrd_create(&T, thrdWorker, &State);
  dl_iterate_phdr(visitObj, &State);
  return nftw(".", visitFile, 4, 0);
}
