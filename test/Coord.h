// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <cstring>

class Coord {

public:

	Coord() : x(0), y(0) {}

	Coord(unsigned int x, unsigned int y) : x(x), y(y) {}

	Coord(const Coord &other) {
		x=other.x;
		y=other.y;

		fprintf(stderr, "Copy %d, %d\n", x, y);
	}

	Coord(Coord &&other) {
		x=other.x;
		y=other.y;

		fprintf(stderr, "Move %d, %d\n", x, y);
	}

	void toJS(nbind::cbOutput output) {
		output(x, y);
	}

	unsigned int x;
	unsigned int y;

};
