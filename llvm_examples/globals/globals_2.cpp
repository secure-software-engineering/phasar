int globalInt = 10;

struct Base {
	virtual void foo(int &a) { 
		a = globalInt; 
	}
};

struct Child : Base {
	void foo(int &a) override { globalInt+10; }
};

int main()
{
	Child c;
	int i = 20;
	c.foo(i);
	Base b;
	b.foo(i);
	return 0;
}