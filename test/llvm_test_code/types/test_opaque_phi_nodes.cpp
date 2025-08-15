struct TypeA {
  int a;
};
struct TypeB {
  int b;
};

TypeA *create_node_a() { return new TypeA{42}; }
TypeA *get_static_a() {
  static TypeA a;
  return &a;
}

void test_phi_scenarios(bool condition, int index) {
  TypeA obj_a;
  TypeB obj_b;
  TypeA array_a[10];
  TypeB array_b[10];

  // Phi node merging different pointer types (all 'ptr' in IR)
  void *result = condition ? (void *)&obj_a : &obj_b; // phi ptr

  // Phi with array access
  TypeA *ptr_a = condition ? &array_a[0] : &array_a[index]; // phi ptr

  // Complex phi in loop
  TypeA *current = &array_a[0];
  for (int i = 0; i < 5; i++) {
    current = &array_a[i]; // phi merging different GEPs -> ptr
  }

  // Phi merging function returns
  TypeA *func_result = condition ? create_node_a() : get_static_a(); // phi ptr
}
