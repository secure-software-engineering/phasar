struct S {
  int data;
  S(int data) : data(data) {}
  ~S() {}
};

S s(0);

// @llvm.global_ctors stores _GLOBAL__sub_I_example.cpp
// _GLOBAL__sub_I_example.cpp calls __cxx_global_var_init
// __cxx_global_var_init calls S::S(int) and cxa_atexit(S::~S(), ...)
// the call to cxa_atexit registers S::~S() to be called after main

int main() {}
