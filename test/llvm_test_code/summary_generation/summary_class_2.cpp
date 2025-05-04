class Point {
private:
  int x;
  int y;

public:
  Point(int x, int y) : x(x), y(y) {}
  int getX() { return x; }
  int getY() { return y; }
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
