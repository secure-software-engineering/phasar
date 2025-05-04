// void fread(int f) {};

void fread(void *ptr) {}
void fwrite(int i) {}

int main(int argc, char **argv) {
  int i = argc;
  int j = argc;
  j = 42;
  int k;
  int l;
  fread(&l);
  fwrite(i);
  return 0;
}
