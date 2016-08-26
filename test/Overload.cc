// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

class Overload {

public:

	unsigned int test(unsigned int x) const { return(1); }
	unsigned int test(unsigned char *x) { return(2); }

	static unsigned int testStatic(unsigned int x) { return(1); }
	static unsigned int testStatic(unsigned char *x) { return(2); }

};

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(Overload) {
	construct<>();

	method(test, nbind::Overloaded<unsigned int(unsigned int) const>());
	method(testStatic, nbind::Overloaded<unsigned int(unsigned int)>());
}

#endif
