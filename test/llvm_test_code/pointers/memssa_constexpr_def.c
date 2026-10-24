// The single reaching def of the load stores a ConstantExpr GEP. That value
// cannot be aliased with, but its leaves can still be assigned from, so R must
// get A only -- not the B that the indirect store also put into P.
int A[4];
int B[4];
int *P;
int *Q;

int main(int Argc, char **Argv) {
  int **Sel = Argc ? &P : &Q;
  *Sel = &B[1];
  P = &A[2];
  int *R = P;
  return *R;
}
