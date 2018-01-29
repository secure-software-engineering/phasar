#ifndef SRC1_H_
#define SRC1_H_

struct A {
<<<<<<< HEAD
	virtual int id(int i);
};

struct B : A {
	virtual int id(int i) override;
};

#endif
=======
	virtual int foo(int &i);
	virtual void bar(double &d);
};

#endif
>>>>>>> const-analysis
