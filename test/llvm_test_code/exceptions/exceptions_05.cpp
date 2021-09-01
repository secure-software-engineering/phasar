// Intra LCA Example

extern void print(int);

int main() {
  int x = 42;
  try {
    throw 34;
  } catch (int y) {
    x = y;
  }

  print(x);
}
