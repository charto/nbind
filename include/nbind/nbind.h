// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#if defined(BUILDING_NODE_EXTENSION) || defined(EMSCRIPTEN)

#include "BindDefiner.h"
#include "FunctionDefiner.h"

// Support overloading macros by number of arguments.
// See http://stackoverflow.com/a/16683147/16509

// VS C++ workaround.
#define VA_EXPAND(args) args

// _1, _2... are dummy parameters to discard original macro arguments.
#define VA_SIZE_HELPER(_6, _5, _4, _3, _2, _1, ARGC, ...) ARGC
#define VA_SIZE(...) VA_EXPAND(VA_SIZE_HELPER(__VA_ARGS__, n, n, n, n, n, 1))

#define VA_CONCAT(A, B) A ## B

#define VA_SELECT_HELPER(name, argc) VA_CONCAT(name ## _, argc)
#define VA_SELECT(name, ...) VA_SELECT_HELPER(name, VA_EXPAND(VA_SIZE(__VA_ARGS__)))(__VA_ARGS__)

#define NBIND_UNIQUE(name, line) VA_CONCAT(name, line)

#define NBIND_GLOBAL() namespace

#define function_1(name) nbind::FunctionDefiner NBIND_UNIQUE(definer, __LINE__)(#name, &name)
#define function_n(name, ...) nbind::FunctionDefiner NBIND_UNIQUE(definer, __LINE__)(#name, &name, __VA_ARGS__)
#define function(...) VA_SELECT(function, __VA_ARGS__)

// Define bindings for a C++ class using a syntax that looks like a function definition.

#define NBIND_CLASS_1(Name) \
	template<class Bound> struct BindInvoker##Name { \
		BindInvoker##Name(); \
		nbind::BindDefiner<Name> definer; \
	}; \
	static struct BindInvoker##Name<Name> bindInvoker##Name; \
	template<class Bound> BindInvoker##Name<Bound>::BindInvoker##Name():definer(#Name)

#define NBIND_CLASS_n(Name, visible) \
	template<class Bound> struct BindInvoker##visible { \
		BindInvoker##visible(); \
		nbind::BindDefiner<Name> definer; \
	}; \
	static struct BindInvoker##visible<Name> bindInvoker##visible; \
	template<class Bound> BindInvoker##visible<Bound>::BindInvoker##visible():definer(#visible)

#define NBIND_CLASS(...) VA_SELECT(NBIND_CLASS, __VA_ARGS__)

// Define a method by name in C++, and optionally a different name visible to JavaScript.

#define method_1(name) definer.method(#name, &Bound::name)
#define method_n(name, ...) definer.method(#name, &Bound::name, __VA_ARGS__)
#define method(...) VA_SELECT(method, __VA_ARGS__)

// Define a constructor.
// Constructor argument types must be appended as template arguments like:
// construct<int, int>()

#define construct definer.constructor

#define field(name) definer.field(#name, &Bound::name)
#define getter(name) definer.property(#name, &Bound::name)
// TODO: varargs macro also supporting this:
//#define getter(name, get) definer.property(#name, &Bound::get)
#define getset(getName, setName) definer.property(#getName, &Bound::getName, &Bound::setName)
// TODO: varargs macro also supporting this:
// #define getset(name, get, set) definer.property(#name, &Bound::get, &Bound::set)

#endif // BUILDING_NODE_EXTENSION || EMSCRIPTEN
