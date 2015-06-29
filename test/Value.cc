// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <cstring>

#include "nbind/Binding.h"

class Coord {

public:

	Coord() : x(0), y(0) {}

	Coord(unsigned int x, unsigned int y) : x(x), y(y) {}

	Coord(const Coord &other) {
x=other.x;
y=other.y;
		fprintf(stderr, "Copy %d, %d\n", x, y);
	}

	Coord(Coord &&other) {
x=other.x;
y=other.y;
		fprintf(stderr, "Move %d, %d\n", x, y);
	}

	void toJS(nbind::cbOutput output) {
		output(x, y);
	}

	unsigned int x;
	unsigned int y;

};

class Value {

public:

	Value() {}

	static Coord getCoord() {
fprintf(stderr, "MOI\n");
		return(Coord(1,2));
	}

	static void foo(Coord a) {
		fprintf(stderr, "%d, %d\n", a.x, a.y);
	}

};

#include "nbind/BindingShort.h"

#ifdef NBIND_CLASS

NBIND_CLASS(Coord) {
	construct<unsigned int, unsigned int>();
}

NBIND_CLASS(Value) {
	construct<>();

	method(getCoord);
	method(foo);
}

#endif
