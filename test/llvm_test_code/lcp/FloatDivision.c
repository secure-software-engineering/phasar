int main() {
  float i = 1.3;
  float j = 8.0;
  float k = 0.2;
  j = k / 0.2; // j = 1
  i = j * k;   // i = TOP
  k = j - 8.0; // k = -7
  return k;
}
