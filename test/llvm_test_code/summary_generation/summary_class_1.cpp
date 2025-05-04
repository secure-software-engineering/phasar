class Point {
private:
  int x;
  int y;

public:
  Point(int x, int y) : x(x), y(y) {}
  int getX() { return x; }
  int getY() { return y; }
};

void pseudo_user() {
  Point p(1, 2);
  p.getX();
  p.getY();
}
