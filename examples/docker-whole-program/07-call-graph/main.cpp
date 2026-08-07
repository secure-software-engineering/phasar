struct Shape  { virtual double area() const { return 0.0; } virtual ~Shape() = default; };
struct Circle : Shape { double area() const override { return 3.14; } };
struct Square : Shape { double area() const override { return 4.0;  } };
static double total(const Shape *s) { return s->area(); } /* virtual call site */
int main() {
    Circle c; Square s;
    return (int)(total(&c) + total(&s));
}
