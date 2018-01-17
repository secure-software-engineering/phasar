int main() {
  int *array = new int[10]; // array is assigned 40 bytes in the heap
  delete[] array;
  return 0;
}