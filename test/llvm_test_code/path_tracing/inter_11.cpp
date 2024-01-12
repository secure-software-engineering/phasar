struct One {
  virtual int assignValue(int J) = 0;
};

struct Two : public One {
  int assignValue(int J) override { return J + 1; }
};

struct Three : public One {
  int assignValue(int J) override { return J + 2; }
};

int main(int Argc, char **Argv) {
  One *O = new Two;
  return O->assignValue(42);
}
