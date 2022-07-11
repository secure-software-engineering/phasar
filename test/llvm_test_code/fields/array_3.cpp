
void sanitize(char *text) { text[2] = 'C'; }

int main(int argc, char **argv) {
  char text[512];
  text[0] = 'A';
  text[1] = 'B';
  text[2] = *argv[0];
}
