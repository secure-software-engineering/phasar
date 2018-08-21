extern bool cond;
int main() {
	int j = 10;
  int i = j;
	if (cond) {
    i = j + 10;
  }
  return 0;
}