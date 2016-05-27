// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <array>
#include <vector>

#include "nbind/api.h"

class Array {

public:

	Array() {}

	static std::array<int, 3> getInts() {
		std::array<int, 3> a {{ 13, 21, 34 }};

		return(a);
	}

	static std::array<int, 3> callWithInts(nbind::cbFunction &callback, std::array<int, 3> a) {
		return(callback.call<std::array<int, 3>>(a));
	}

};

class Vector {

public:

	Vector() {}

	static std::vector<int> getInts() {
		std::vector<int> a {{ 13, 21, 34 }};

		return(a);
	}

	static std::vector<int> callWithInts(nbind::cbFunction &callback, std::vector<int> a) {
		return(callback.call<std::vector<int>>(a));
	}

	static std::vector<std::string> callWithStrings(nbind::cbFunction &callback, std::vector<std::string> a) {
		return(callback.call<std::vector<std::string>>(a));
	}

};

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(Array) {
	construct<>();

	method(getInts);
	method(callWithInts);
}

NBIND_CLASS(Vector) {
	construct<>();

	method(getInts);
	method(callWithInts);
	method(callWithStrings);
}

#endif
