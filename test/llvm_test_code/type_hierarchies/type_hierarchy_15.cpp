
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


int main() {
    A a;
    a.foo();
    B b;
    b.foo();
    b.bar();
    b.quack();
    return 0;
}
