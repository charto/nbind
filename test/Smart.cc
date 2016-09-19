// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <memory>
#include <cstdio>

class Smart {

public:

	Smart(int num) : num(num) {}

	~Smart() {
		fprintf(stderr, "Destroy %d!!!\n", num);
	}

	static std::shared_ptr<Smart> make(int num) {
		return(std::make_shared<Smart>(num));
	}

	void test() {
		fprintf(stderr, "test %d!!!\n", num);
	}

	static void testStatic(Smart *ptr) {
		fprintf(stderr, "static %d!!!\n", ptr->num);
	}

	static void testShared(std::shared_ptr<Smart> ptr) {
		fprintf(stderr, "shared %d!!!\n", ptr->num);
	}

private:

	int num;

};

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(Smart) {
	method(make);

	method(test);
	method(testStatic);
	method(testShared);
}

#endif
