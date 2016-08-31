// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

class Overload {

public:

	unsigned int test(unsigned int x) { return(1); }
	unsigned int test(unsigned int x, unsigned int y) { return(2); }

	unsigned int testConst(unsigned int x) const { return(1); }
	unsigned int testConst(unsigned int x, unsigned int y) const { return(2); }

	static unsigned int testStatic(unsigned int x) { return(1); }
	static unsigned int testStatic(unsigned int x, unsigned int y) { return(2); }

};

unsigned int multiTest(unsigned int x) { return(1); }
unsigned int multiTest(unsigned int x, unsigned int y) { return(2); }

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(Overload) {
	construct<>();

	multimethod(test, args(unsigned int));
	multimethod(test, args(unsigned int, unsigned int), "test2");

	multimethod(testConst, args(unsigned int));
	multimethod(testConst, args(unsigned int, unsigned int), "testConst2");

	multimethod(testStatic, args(unsigned int));
	multimethod(testStatic, args(unsigned int, unsigned int), "testStatic2");
}

NBIND_GLOBAL() {
	multifunction(multiTest, args(unsigned int));
	multifunction(multiTest, args(unsigned int, unsigned int), "multiTest2");
}

#endif
