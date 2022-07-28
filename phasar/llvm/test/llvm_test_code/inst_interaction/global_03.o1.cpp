int GlobalFeature = 2;

int main(int argc, char *argv[]) {
  int Sum = argc;

  switch (GlobalFeature + argc) {
  case 0:
    return 21;
  case 1:
    Sum += 2;
  case 7:
    Sum += 3;
    break;
  case 2:
    Sum += 4;
    break;
  case 4:
    Sum += 4;
    break;
  }

  Sum += 42;

  return Sum;
}
