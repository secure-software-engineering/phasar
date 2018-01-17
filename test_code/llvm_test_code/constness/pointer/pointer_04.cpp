int main() {
  int *ptr = new int(42); // ptr is assigned 4 bytes in the heap
  delete ptr;
  return 0;
}
