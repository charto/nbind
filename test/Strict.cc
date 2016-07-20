// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <cstring>
#include <string>

class Strict {

public:

	static int testInt(int x) { return(x); }
	static std::string testString(std::string x) { return(x); }
	static const char *testCString(const char *x) { return(x); }

};

int testInt(int x) { return(x); }
std::string testString(std::string x) { return(x); }
const char *testCString(const char *x) { return(x); }

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(Strict) {
	method(testInt);
	method(testString);
	method(testCString);

	method(testInt, "strictInt", nbind::Strict());
	method(testString, "strictString", nbind::Strict());
	method(testCString, "strictCString", nbind::Strict());
}

NBIND_GLOBAL() {
	function(testInt);
	function(testString);
	function(testCString);

	function(testInt, "strictInt", nbind::Strict());
	function(testString, "strictString", nbind::Strict());
	function(testCString, "strictCString", nbind::Strict());
}

#endif
