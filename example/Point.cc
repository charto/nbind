// This file is part of nbind, copyright (C) 2014 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include "Point.h"

#ifdef BUILDING_NODE_EXTENSION
#include "nbind/Binding.h"

using namespace nbind;

NODEJS_BINDINGS(Point) {
	class_<Point>("Point")
		.constructor<>()
		.constructor<double,double>()
		.function("add",&Point::add)
		.function("print",&Point::print);
}

NODE_MODULE(example,Bindings::initModule)

#endif
