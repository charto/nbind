// This file is part of nbind, copyright (C) 2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles type conversion of C++ standard library types
// to / from JavaScript.

#pragma once

#include <functional>

namespace nbind {

#define NBIND_DEFINE_CALLBACK_TYPE(ArgType)         \
template <typename ReturnType, typename... Args>    \
struct BindingType<ArgType> {                       \
                                                    \
	typedef ArgType Type;                           \
                                                    \
	typedef unsigned int WireType;                  \
                                                    \
};                                                  \
                                                    \
template<typename PolicyList, typename ReturnType, typename... Args> \
struct ArgFromWire<PolicyList, ArgType> {           \
                                                    \
	explicit ArgFromWire(unsigned int num) : val(cbWrapper<ReturnType>(num)) {} \
                                                    \
	inline ArgType get(unsigned int num) {          \
		return(val);                                \
	}                                               \
                                                    \
	std::function<ReturnType (Args...)> val;        \
                                                    \
}

NBIND_DEFINE_CALLBACK_TYPE(std::function<ReturnType (Args...)>);
NBIND_DEFINE_CALLBACK_TYPE(std::function<ReturnType (Args...)> &);
NBIND_DEFINE_CALLBACK_TYPE(std::function<ReturnType (Args...)> const &);

} // namespace
