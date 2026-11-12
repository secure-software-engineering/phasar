// Function pointers that only occur in global initializers

typedef int (*FnTy)(int);

int glob(int X) { return X; }
int arr0(int X) { return X + 1; }
int arr1(int X) { return X + 2; }
int disp0(int X) { return X + 3; }
int disp1(int X) { return X + 4; }
int other(int X) { return X + 5; }

struct Section {
  const char *Name;
  long Size;
};

struct Display {
  struct Section Sec;
  FnTy Fn;
  int Flag;
};

FnTy GlobFP = &glob;
FnTy GlobArr[2] = {arr0, arr1};
struct Display Displays[2] = {{{"a", 1}, disp0, 0}, {{"b", 2}, disp1, 1}};

FnTy *ArrPtr = &GlobArr[1];

// Makes other address-taken, so the base call-graph considers it
FnTy OtherFP = &other;

int callGlob(int X) { return GlobFP(X); }

int callArr(int X) { return GlobArr[X % 2](X); }

int callDisp(int X) {
  struct Display *Disp = &Displays[X % 2];
  return Disp->Fn(X);
}

int callArrPtr(int X) { return (*ArrPtr)(X); }

int main(int Argc, char **Argv) {
  return callGlob(Argc) + callArr(Argc) + callDisp(Argc) + callArrPtr(Argc);
}
