/* immutable: multi1 */
int main() {
  int multi1[2][1] = {{71},{81}};
  int multi2[1][2] = {17, 18};
  multi2[0][1] = 99;
  return 0;
}