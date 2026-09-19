struct Payload {
  long A;
  long B;
  long C;
};

long consume(Payload P) { return P.A + P.B + P.C; }
