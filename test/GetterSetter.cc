// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <cstring>

class GetterSetter {

public:

	GetterSetter() {}

	int get_x() {return(x);}
	int Gety() {return(y);}
	int getZ() const {return(z);}

	const char *gett() const {return(t);}

	void Sety(int y) {this->y = y;}
	void setZ(int z) {this->z = z;}

	void sett(const char *t) {this->t = strdup(t);}

	int getXYZ() const { return(x + y + z); }

private:

	int x = 1, y = 2, z = 3;
	const char *t = "foobar";

};

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(GetterSetter) {
	construct<>();

	getter(get_x);
	getset(Gety, Sety);
	getset(getZ, setZ);
	getset(gett, sett);
	getter(getXYZ);
}

#endif
