// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <cstring>
#include <string>

#include "nbind/api.h"

class Callback {

public:

	Callback() {}

	static void callVoidFunc(nbind::cbFunction &voidFunc) {voidFunc.call<void>();}

	static bool callNegate(nbind::cbFunction &negate, bool x) {return(negate.call<bool>(x));}

	static int callIncrementInt(nbind::cbFunction &incrementInt, int x) {return(incrementInt.call<int>(x));}

	static double callIncrementDouble(nbind::cbFunction &incrementDouble, double x) {return(incrementDouble.call<double>(x));}

	static std::string callCatenate(nbind::cbFunction &catenate, const char *a, const char *b) {return(catenate.call<std::string>(a, b));}

	static void callCStrings(nbind::cbFunction &cb) {
		std::string foo = "foo";
		std::string bar = "bar";
		std::string baz = "baz";
		cb(foo, bar, baz);
	}

};

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(Callback) {
	construct<>();

	method(callVoidFunc);
	method(callNegate);
	method(callIncrementInt);
	method(callIncrementDouble);
	method(callCatenate);
	method(callCStrings);
}

#endif
