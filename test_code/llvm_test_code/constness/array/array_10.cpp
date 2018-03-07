/* immutable: p */
int main() {
  int arr[2] = {42, 24};
  int *p = arr;  // int *p = &arr[0]
  *(p+1) = 17;   // arr[1] = 17
  return 0;
}