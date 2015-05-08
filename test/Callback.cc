// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <cstring>

#include "nbind/BindingShort.h"

class Callback {

public:

	Callback() {}

	static bool callNegate(nbind::cbFunction negate, bool x) {return(negate.call<bool>(x));}

	static int callIncrementInt(nbind::cbFunction incrementInt, int x) {return(incrementInt.call<int>(x));}

//	static void (*incrementState)(nbind::cbFunction);

//	static int (*getState)(nbind::cbFunction);

};

#ifdef NBIND_CLASS

NBIND_CLASS(Callback) {
	construct<>();

	method(callNegate);
	method(callIncrementInt);
}

#endif
