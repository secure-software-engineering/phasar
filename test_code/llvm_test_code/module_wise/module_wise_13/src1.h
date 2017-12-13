#ifndef SRC1_H_
#define SRC1_H_

struct A {
	virtual int id(int i);
};

struct B : A {
	virtual int id(int i) override;
};

#endif
