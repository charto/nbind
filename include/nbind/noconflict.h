// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#if defined(BUILDING_NODE_EXTENSION) || defined(__EMSCRIPTEN__)

#include "BindDefiner.h"
#include "FunctionDefiner.h"

// Support overloading macros by number of arguments.
// See http://stackoverflow.com/a/16683147/16509

// VS C++ workaround.
#define NBIND_EXPAND(args) args

// _1, _2... are dummy parameters to discard original macro arguments.
#define NBIND_SIZE_HELPER(_6, _5, _4, _3, _2, _1, ARGC, ...) ARGC
#define NBIND_SIZE(...) NBIND_EXPAND(NBIND_SIZE_HELPER(__VA_ARGS__, n, n, n, n, n, 1))

#define NBIND_CONCAT(a, b) a ## b

#define NBIND_SELECT_HELPER(name, argc) NBIND_CONCAT(name ## _, argc)
#define NBIND_SELECT(name, ...) NBIND_SELECT_HELPER(name, NBIND_EXPAND(NBIND_SIZE(__VA_ARGS__)))(__VA_ARGS__)

#define NBIND_UNIQUE(name, line) NBIND_CONCAT(name, line)

#define NBIND_GLOBAL() namespace

#define NBIND_FUNCTION(name, ...) nbind::FunctionDefiner NBIND_UNIQUE(definer, __LINE__)(#name, &name, ## __VA_ARGS__)

#define NBIND_MULTIFUNCTION(name, args, ...) nbind::FunctionDefiner :: template Overloaded args NBIND_UNIQUE(definer, __LINE__)(#name, &name, ## __VA_ARGS__)

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

#define NBIND_CLASS(...) NBIND_SELECT(NBIND_CLASS, __VA_ARGS__)

// Define a method by name in C++, and optionally a different name visible to JavaScript.

#define NBIND_METHOD(name, ...) definer.method(#name, &Bound::name, ## __VA_ARGS__)

#define NBIND_INHERIT(Name) definer.inherit<Name>()

#define NBIND_ARGS(...) <__VA_ARGS__>

#ifndef NBIND_MULTIMETHOD
#define NBIND_MULTIMETHOD(name, args, ...) definer.overloaded args ().method(#name, &Bound::name, ## __VA_ARGS__)
#endif

// Define a constructor.
// Constructor argument types must be appended as template arguments like:
// construct<int, int>()

#define NBIND_CONSTRUCT definer.constructor

#define NBIND_FIELD(name) definer.field(#name, &Bound::name)
#define NBIND_GETTER(name, ...) definer.property(#name, &Bound::name, ## __VA_ARGS__)
#define NBIND_GETSET(getName, setName, ...) \
definer.property(#getName, &Bound::getName, &Bound::setName, ## __VA_ARGS__)

#define NBIND_ALIAS(Name, base) \
namespace nbind { \
template<> struct BindingType<Name> : BindingType<base> { \
	static inline Name fromWireType(WireType arg) { \
		return(static_cast<Name>(BindingType<Type>::fromWireType(arg))); \
	} \
	static inline WireType toWireType(Name arg) { \
		return(BindingType<Type>::toWireType(static_cast<Type>(arg))); \
	} \
}; \
template<> struct Typer<Name> : Typer<base> {}; \
}


#endif // BUILDING_NODE_EXTENSION || __EMSCRIPTEN__
