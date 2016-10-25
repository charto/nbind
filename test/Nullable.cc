// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <cstdio>

#include "nbind/api.h"
#include "Coord.h"

class Nullable {

public:

	static Coord *getCoord() {
		return(new Coord(60, 25));
	}

	static Coord *getNull() {
		return(nullptr);
	}

	static void foo(Coord *coord) {
		printf("%p\n", coord);
	}

	static void bar(Coord *coord) {
		printf("%p\n", coord);
	}

};

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(Nullable) {
	method(getCoord);
	method(getNull);
	method(foo);
	method(bar, "bar", nbind::Nullable());
}

#endif
