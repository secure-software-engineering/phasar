#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  size_t bytes;
  FILE *F = fopen("taint_1.ll", "r");
  if (F) {
    fseek(F, 0, SEEK_END);
    bytes = ftell(F);
    fseek(F, 0, SEEK_SET);
    printf("The file is %zu bytes.\n", bytes);
    char *buffer = (char *)malloc(bytes + 1);
    fread(buffer, sizeof(char), bytes, F);
    buffer[bytes] = '\0';
    printf("Read:\n%s", buffer);
    free(buffer);
  }
  if (fclose(F)) {
    printf("Could not close file properly.\n");
  }
  return 0;
}
