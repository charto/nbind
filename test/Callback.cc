// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <cstring>
#include <string>

#include "nbind/api.h"

class Callback {

public:

	Callback() {}

	static void callVoidFunc(nbind::cbFunction &voidFunc) {voidFunc.call<void>();}
	static void callVoidFunc2(std::function<void ()> voidFunc) { voidFunc(); }

	static bool callNegate(nbind::cbFunction &negate, bool x) {return(negate.call<bool>(x));}
	static bool callNegate2(std::function<bool (bool)> negate, bool x) {return(negate(x));}

	static int callAddInt(const nbind::cbFunction &incrementInt, int x, int y) {return(incrementInt.call<int>(x, y));}
	static int callAddInt2(const std::function<int (int, int)> &incrementInt, int x, int y) {return(incrementInt(x, y));}

	static double callIncrementDouble(const nbind::cbFunction &incrementDouble, double x) {return(incrementDouble.call<double>(x));}

	static std::string callCatenate(nbind::cbFunction &catenate, const char *a, const char *b) {return(catenate.call<std::string>(a, b));}
	static std::string callCatenate2(nbind::cbFunction &catenate, const char *a, const char *b) {return(catenate.call<std::string>(a, b));}

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
	method(callVoidFunc2);

	method(callNegate);
	method(callNegate2);

	method(callAddInt);
	method(callAddInt2);

	method(callIncrementDouble);

	method(callCatenate);
	method(callCatenate2);

	method(callCStrings);
}

#endif
