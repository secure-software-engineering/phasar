class Point {
private:
  int x;
  int y;
  int identity(int i) { return i; }

public:
  Point(int x, int y) : x(x), y(y) {}
  int getX() { return identity(x); }
  int getY() { return identity(y); }
  void setX(int a) { x = a; }
  void setY(int b) { y = b; }
};

void pseudo_user() {
  Point p(1, 2);
  p.setX(13);
  p.setY(42);
  p.getX();
  p.getY();
}
