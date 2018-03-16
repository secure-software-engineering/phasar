/* immutable: p */
int main() {
  int *p = new int(42); 
  *p = 20;
  delete p;
  return 0;
}
