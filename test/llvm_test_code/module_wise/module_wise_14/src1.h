#ifndef SRC1_H_
#define SRC1_H_
// similar to module_wise_13 for testing projects with
// overlaping functions and types
struct A {
  virtual int foo(int &i);
  virtual void bar(double &d);
};

#endif
