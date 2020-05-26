// cfgdemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

class A {
public:
	virtual ~A() = default;
	virtual void DoWork(int x) {
		printf("A::DoWork %d\n", x);
	}
};

class B : public A {
public:
	void DoWork(int x) override {
		printf("B::DoWork %d\n", x);
	}
};


int main() {
	A a;
	a.DoWork(10);
	B b;
	b.DoWork(20);

	A* pA = new B;
	pA->DoWork(30);

	delete pA;
}

