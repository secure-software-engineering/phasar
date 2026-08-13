// After mem2reg the loop pointer is a PHI whose second incoming value is the
// load below. handlePhi interns the load first, so handleLoad's addAlias()
// no-ops and -- on the single-reaching-def MemSSA path -- the load is left
// without any incoming edge at all, aliasing nothing.
int A;
int B;
int *Slot;

int main(int Argc, char **Argv) {
  Slot = &A;
  int *P = &B;
  for (int I = 0; I < Argc; ++I) {
    P = Slot;
  }
  return *P;
}
