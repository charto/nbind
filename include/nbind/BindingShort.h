// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#ifdef BUILDING_NODE_EXTENSION

#include "Binding.h"

#define NBIND_ERR(message) nbind::Bindings::setError(message)

#define NBIND_CLASS(Name) \
	template<class Bound> struct BindInvoker { \
		BindInvoker(); \
		nbind::BindClass<Name> linkage; \
		nbind::BindDefiner<Name> definer; \
	}; \
	static struct BindInvoker<Name> bindInvoker##Name; \
	template<class Bound> BindInvoker<Bound>::BindInvoker():definer(#Name)

#define method(name) definer.function(#name, &Bound::name)
#define construct definer.constructor

#define NBIND_INIT(moduleName) NODE_MODULE(moduleName, nbind::Bindings::initModule)

#else

#define NBIND_ERR(message)

#endif
