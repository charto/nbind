// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <cstring>
#include <string>

class Strict {

public:

	int testInt(int x) { return(x); }
	bool testBool(bool x) { return(x); }
	std::string testString(std::string x) { return(x); }
	const char *testCString(const char *x) { return(x); }

	int strictInt(int x) { return(x); }

};

class StrictStatic {

public:

	static int testInt(int x) { return(x); }
	static bool testBool(bool x) { return(x); }
	static std::string testString(std::string x) { return(x); }
	static const char *testCString(const char *x) { return(x); }

	static int strictInt(int x) { return(x); }

};

int testInt(int x) { return(x); }
bool testBool(bool x) { return(x); }
std::string testString(std::string x) { return(x); }
const char *testCString(const char *x) { return(x); }

int strictInt(int x) { return(x); }

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(Strict) {
	construct<>();

	method(testInt);
	method(testBool);
	method(testString);
	method(testCString);

	method(strictInt, nbind::Strict());
	method(testBool, "strictBool", nbind::Strict());
	method(testString, "strictString", nbind::Strict());
	method(testCString, "strictCString", nbind::Strict());
}

NBIND_CLASS(StrictStatic) {
	method(testInt);
	method(testBool);
	method(testString);
	method(testCString);

	method(strictInt, nbind::Strict());
	method(testBool, "strictBool", nbind::Strict());
	method(testString, "strictString", nbind::Strict());
	method(testCString, "strictCString", nbind::Strict());
}

NBIND_GLOBAL() {
	function(testInt);
	function(testBool);
	function(testString);
	function(testCString);

	function(strictInt, nbind::Strict());
	function(testBool, "strictBool", nbind::Strict());
	function(testString, "strictString", nbind::Strict());
	function(testCString, "strictCString", nbind::Strict());
}

#endif
