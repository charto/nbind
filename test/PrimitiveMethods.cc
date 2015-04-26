// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <cstring>

class PrimitiveMethods {

public:

	PrimitiveMethods() {}
	~PrimitiveMethods() {
		state++;
	}

	static bool negateStatic(bool x) {return(!x);}
	bool negate(bool x) {return(!x);}

	static int incrementIntStatic(int x) {return(x+1);}
	int incrementInt(int x) {return(x+1);}

	static void incrementStateStatic() {state++;}
	void incrementState() {state++;}

	static int getStateStatic() {return(state);}
	int getState() {return(state);}

	static int strLengthStatic(const char *x) {return(strlen(x));}
	int strLength(const unsigned char *x) {return(strlen(reinterpret_cast<const char *>(x)));}

private:

	static int state;

};

int PrimitiveMethods::state = 0;

#include "nbind/BindingShort.h"

#ifdef NBIND_CLASS

NBIND_CLASS(PrimitiveMethods) {
	construct<>();

	method(negateStatic);
	method(negate);

	method(incrementIntStatic);
	method(incrementInt);

	method(incrementStateStatic);
	method(incrementState);

	method(getStateStatic);
	method(getState);

	method(strLengthStatic);
	method(strLength);
}

#endif
