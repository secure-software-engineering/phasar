struct Point {
  int x, y;
};

struct Line {
  Point start, end;
};

void test_pointer_arithmetic() {
  Point points[10];
  Line lines[5];

  // These all return 'ptr' in opaque pointer IR
  Point *p1 = &points[3];      // GEP -> ptr
  Point *p2 = p1 + 2;          // GEP -> ptr
  Point *p3 = &lines[1].start; // GEP with struct access -> ptr

  int *x_ptr = &p1->x;       // GEP to member -> ptr
  int *y_ptr = &points[5].y; // Complex GEP -> ptr

  // Pointer arithmetic chains
  Point *complex = &((p1 + 1)[2]); // Multiple operations -> ptr
}
