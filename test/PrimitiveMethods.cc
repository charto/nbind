// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <cstring>
#include <string>

class PrimitiveMethods {

public:

	PrimitiveMethods() { PrimitiveMethods::state = 42; }
	PrimitiveMethods(int state) { PrimitiveMethods::state = state; }
	PrimitiveMethods(int state, std::string str) {
		PrimitiveMethods::state = state;
		strcpy(buf, str.c_str());
	}

	~PrimitiveMethods() {
		state++;
	}

	static bool negateStatic(bool x) {return(!x);}
	bool negate(bool x) const {return(!x);}

	static int incrementIntStatic(int x) {return(x+1);}
	int incrementInt(int x) const {return(x+1);}

	static void incrementStateStatic() {state++;}
	void incrementState() {state++;}

	static int getStateStatic() {return(state);}
	int getState() {return(state);}

	static std::string getStringStatic() {return(buf);}
	std::string getString() {return(buf);}

	static int strLengthStatic(const char *x) {return(strlen(x));}
	int strLength(const unsigned char *x) {return(strlen(reinterpret_cast<const char *>(x)));}

	static char *catenateStatic(const char *x, const char *y) {
		strcpy(buf, x);
		strcat(buf, y);
		return(buf);
	}

	unsigned char *catenate(const unsigned char *x, const unsigned char *y) {
		strcpy(buf, (char *)x);
		strcat(buf, (char *)y);
		return((unsigned char *)buf);
	}

	std::string catenate2(const std::string &x, const std::string &y) {
		return(x + y);
	}

	template <typename T>
	static T toInt(double x) {
		T y = x;

		return((y < 0) ? y + 1 : y - 1);
	}

	template <typename T>
	static double toFloat(T x) {
		return((x < 0) ? x - 1 : x + 1);
	}

	static long ftol(double x) { return(toInt<long>(x)); }
	static unsigned long ftoul(double x) { return(toInt<unsigned long>(x)); }
	static long long ftoll(double x) { return(toInt<long long>(x)); }
	static unsigned long long ftoull(double x) { return(toInt<unsigned long long>(x)); }

	static double ltof(long x) { return(toFloat(x)); }
	static double ultof(unsigned long x) { return(toFloat(x)); }
	static double lltof(long long x) { return(toFloat(x)); }
	static double ulltof(unsigned long long x) { return(toFloat(x)); }

private:

	static char buf[12];
	static int state;

};

int PrimitiveMethods::state = 0;
char PrimitiveMethods::buf[12];

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(PrimitiveMethods) {
	construct<>();
	construct<int>();
	construct<int, std::string>();

	method(negateStatic);
	method(negate);

	method(incrementIntStatic);
	method(incrementInt);

	method(incrementStateStatic);
	method(incrementState);

	method(getStateStatic);
	method(getState);

	method(getString);
	method(getStringStatic);

	method(strLengthStatic);
	method(strLength);

	method(catenateStatic);
	method(catenate);
	method(catenate2);

	method(ftol);
	method(ftoul);
	method(ftoll);
	method(ftoull);

	method(ltof);
	method(ultof);
	method(lltof);
	method(ulltof);
}

#endif
