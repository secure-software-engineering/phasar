
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

size_t source([[clang::annotate("psr.source")]] char *Buffer,
              size_t BufferSize) {
  return fread(Buffer, BufferSize, 1, stdin);
}

void sink([[clang::annotate("psr.sink")]] const char *Buf, size_t BufferSize) {
  fwrite(Buf, strnlen(Buf, BufferSize), 1, stdout);
}

void print(const char *Buf1, const char *Buf2, size_t Sz) {

  const char *ToPrint;
  if (rand()) {
    ToPrint = Buf1;
  } else {
    ToPrint = Buf2;
  }

  sink(ToPrint, Sz);
}

int main() {
  char Buffer[128];

  source(Buffer, sizeof(Buffer));
  char AnotherBuffer[128] = "Hello, World!";

  print(Buffer, AnotherBuffer, sizeof(Buffer));
}
