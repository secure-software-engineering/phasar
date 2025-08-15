struct Matrix {
  int data[10][10];
};

struct Container {
  Matrix *matrices[5];
};

void test_complex_expressions() {
  Container containers[3];
  Matrix m1, m2;
  containers[0].matrices[0] = &m1;
  containers[1].matrices[2] = &m2;

  // Very complex GEP chains - all result in 'ptr'
  int *complex1 = &containers[1].matrices[2]->data[3][4];   // Multi-level GEP
  int *complex2 = &(*containers[0].matrices[0]).data[0][1]; // Deref + GEP

  // Indirect access through pointers
  Matrix **matrix_ptr = &containers[2].matrices[3]; // GEP -> ptr to ptr
  int *indirect = &(**matrix_ptr).data[5][6];       // Load + GEP chain

  // Array indexing with complex expressions
  int idx = 7;
  int *dynamic_gep = &containers[idx % 3].matrices[idx % 5]->data[idx][idx + 1];
}
