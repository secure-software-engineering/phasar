struct _point {
  int x, y;
};

typedef struct _point point;
typedef struct _point *ppoint;

extern void generate(ppoint p);

// clang passes point as i64 (ifndef BYREF)
int getX(point
#ifdef BYREF
             *
#endif
                 p) {
  return p
#ifdef BYREF
      ->
#else
      .
#endif
      x;
}

int main() {
  point p;
  generate(&p);
  p = (point){.x = 4, .y = 5};
  int i = getX(
#ifdef BYREF
      &
#endif
      p);
  int j = i * 2;
  return j;
}