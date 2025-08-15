struct Base {
  int b;
};
struct Derived : Base {
  int d;
};

class Polymorphic {
  virtual ~Polymorphic() = default;
  int poly_data;
};

void test_opaque_casts() {
  Derived d;
  char buffer[1024];

  // All these are 'ptr' in IR but different debug types
  Base *base_ptr = &d;                                     // Upcast -> ptr
  Derived *derived_ptr = static_cast<Derived *>(base_ptr); // Downcast -> ptr
  void *void_ptr = &d;                                     // To void* -> ptr
  char *char_ptr = reinterpret_cast<char *>(&d);           // Reinterpret -> ptr
  int *int_ptr = reinterpret_cast<int *>(buffer);          // From array -> ptr

  // Dynamic cast (may involve runtime type checking)
  Polymorphic *poly = new Polymorphic();
  void *generic = poly;
  Polymorphic *back = static_cast<Polymorphic *>(generic); // ptr -> ptr
}
