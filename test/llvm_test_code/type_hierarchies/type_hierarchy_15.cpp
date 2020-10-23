
struct A {
    virtual ~A() = default;
    virtual void foo() {}
    virtual void bar() {}
};

struct B : A{
    ~B() override = default;
    void foo() override {}
    virtual void quack() {}
};

void caller(A& a) {
    a.foo();
    a.bar();
}

int main() {
    A a;
    caller(a);
    B b;
    caller(b);
    return 0;
}
