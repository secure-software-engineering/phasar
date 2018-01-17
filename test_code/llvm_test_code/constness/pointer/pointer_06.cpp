int main() {
  int *ptr = new int(42); // ptr is assigned 4 bytes in the heap
  *ptr = 20;
  delete ptr;
  return 0;
}
