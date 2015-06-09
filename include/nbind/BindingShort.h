// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#ifdef BUILDING_NODE_EXTENSION

#include "Binding.h"

#define NBIND_ERR(message) nbind::Bindings::setError(message)

#define NBIND_CLASS(Name) \
	template<class Bound> struct BindInvoker##Name { \
		BindInvoker##Name(); \
		nbind::BindClass<Name> linkage; \
		nbind::BindDefiner<Name> definer; \
	}; \
	static struct BindInvoker##Name<Name> bindInvoker##Name; \
	template<class Bound> BindInvoker##Name<Bound>::BindInvoker##Name():definer(#Name)

#define method(name) definer.function(#name, &Bound::name)
#define construct definer.constructor
#define field(name) definer.field(#name, &Bound::name)
#define getter(name) definer.property(#name, &Bound::name)
// TODO: varargs macro also supporting this:
//#define getter(name, get) definer.property(#name, &Bound::get)
#define getset(getName, setName) definer.property(#getName, &Bound::getName, &Bound::setName)
// TODO: varargs macro also supporting this:
// #define getset(name, get, set) definer.property(#name, &Bound::get, &Bound::set)

#define NBIND_INIT(moduleName) NODE_MODULE(moduleName, nbind::Bindings::initModule)

#else

#define NBIND_ERR(message)

#endif
