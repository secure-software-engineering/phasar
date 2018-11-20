// Handle function call from inside of eachother

void A() {}
void B() { A(); }
void C() { B(); }
void D() {
  C();
  B();
}
void E() { C(); }

void F() {
  D();
  E();
}

int main(int argc, char **argv) { F(); }