// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

class GetterSetter {

public:

	GetterSetter() : x(1), y(2), z(3) {}

	int get_x() {return(x);}
	int Gety() {return(y);}
	int getZ() {return(z);}

	void setY(int y) {this->y = y;}
	void setZ(int z) {this->z = z;}

private:

	int x, y, z;

};

#include "nbind/BindingShort.h"

#ifdef NBIND_CLASS

NBIND_CLASS(GetterSetter) {
	construct<>();

	getter(get_x);
	getset(Gety, setY);
	getset(getZ, setZ);
}

#endif
