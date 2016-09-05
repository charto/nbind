// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

class Reference {

public:

	Reference() {}

	void read() const {}
	void write() {}

	static Reference getValue() { return(ref); }

	static Reference *getPtr() { return(&ref); }
	static Reference &getRef() { return(ref); }

	static const Reference *getConstPtr() { return(&ref); }
	static const Reference &getConstRef() { return(ref); }

	static void readPtr(const Reference *ref) {}
	static void readRef(const Reference &ref) {}

	static void writePtr(Reference *ref) {}
	static void writeRef(Reference *ref) {}

	static Reference ref;

};

Reference Reference::ref;

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(Reference) {
	construct<>();

	method(read);
	method(write);

	method(getValue);

	method(getPtr);
	method(getRef);

	method(getConstPtr);
	method(getConstRef);

	method(readPtr);
	method(readRef);

	method(writePtr);
	method(writeRef);
}

#endif
