// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <cstring>

#include "nbind/Binding.h"

class Coord {

public:

	Coord(unsigned int x, unsigned int y) : x(x), y(y) {}

	void toJS(nbind::cbOutput output) {
		output(x, y);
	}

private:

	unsigned int x;
	unsigned int y;

};

class Value {

public:

	Value() {}

	static Coord getCoord() {
		return(Coord(1,2));
	}

	static void registerr(nbind::cbFunction func) {
		nbind::BindClass<Coord>::getInstance()->setValueConstructor(func.getJsFunction());
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
	method(registerr);
}

#endif
