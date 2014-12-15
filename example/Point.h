// This file is part of nbind, copyright (C) 2014 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <cstdio>

class Point {

public:

	Point():x(0),y(0) {}
	Point(double x,double y):x(x),y(y) {}

	void add(Point *other) {
		x+=other->x;
		y+=other->y;
	}

	void print() {
		printf("%f, %f\n",x,y);
	}

private:

	double x;
	double y;

};
