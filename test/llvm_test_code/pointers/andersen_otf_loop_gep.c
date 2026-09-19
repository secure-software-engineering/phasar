// After mem2reg the loop pointer is a PHI whose second
// incoming value is the GEP below.  handlePhi interns the GEP first, so
// addPtrAlias's addAlias() call fails and the GEP node keeps an empty
// points-to set instead of aliasing Buf.
char *findEnd(char *Buf) {
  char *P = Buf;
  while (*P != 0) {
    P = P + 1;
  }
  return P;
}

int main() {
  char Buf[16];
  char *E = findEnd(Buf);
  return *E;
}
