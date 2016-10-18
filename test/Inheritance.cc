// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <cstring>
#include <string>

#include <vector>
#include "nbind/api.h"

struct UnBound;

struct InheritanceA {
	unsigned int useA() { return(a); }

	static unsigned int staticA(InheritanceA &a) { return(a.a); }

	unsigned int a = 1;
};

struct InheritanceB : public virtual InheritanceA {
	InheritanceA *getA() { return(static_cast<InheritanceA *>(this)); }

	unsigned int useB() { return(b); }

	static unsigned int staticA(InheritanceB &b) { return(b.a); }
	static unsigned int staticB(InheritanceB &b) { return(b.b); }

	unsigned int b = 2;
};

struct InheritanceC : public virtual InheritanceA {
	InheritanceA *getA() { return(static_cast<InheritanceA *>(this)); }

	unsigned int useC() { return(c); }

	static unsigned int staticA(InheritanceC &c) { return(c.a); }
	static unsigned int staticC(InheritanceC &c) { return(c.c); }

	unsigned int c = 3;
};

struct InheritanceD : public InheritanceB, public InheritanceC {

	InheritanceA *getA() { return(static_cast<InheritanceA *>(this)); }
	InheritanceB *getB() { return(static_cast<InheritanceB *>(this)); }
	InheritanceC *getC() { return(static_cast<InheritanceC *>(this)); }

	unsigned int useD() { return(d); }

	static unsigned int staticA(InheritanceD &d) { return(d.a); }
	static unsigned int staticB(InheritanceD &d) { return(d.b); }
	static unsigned int staticC(InheritanceD &d) { return(d.c); }
	static unsigned int staticD(InheritanceD &d) { return(d.d); }

	unsigned int d = 4;

};

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(InheritanceA) {
	construct<>();

	method(useA);

	method(staticA);
}

NBIND_CLASS(InheritanceB) {
	inherit(InheritanceA);

	construct<>();

	method(useB);
	method(staticA);
	method(staticB);

	getter(getA);
}

NBIND_CLASS(InheritanceC) {
	inherit(InheritanceA);

	construct<>();

	method(useC);
	method(staticA);
	method(staticC);

	getter(getA);
}

NBIND_CLASS(InheritanceD) {
	inherit(InheritanceB);
	inherit(InheritanceC);

	construct<>();

	method(useD);
	method(staticA);
	method(staticB);
	method(staticC);
	method(staticD);

	getter(getA);
	getter(getB);
	getter(getC);
}

#endif
