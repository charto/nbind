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
	static inline bool checkType(WireType arg) {    \
		return(arg->IsFunction());                  \
	}                                               \
                                                    \
};                                                  \
                                                    \
template<typename PolicyList, size_t Index, typename ReturnType, typename... Args> \
struct ArgFromWire<PolicyList, Index, ArgType> {    \
                                                    \
	template <typename NanArgs>                     \
	ArgFromWire(const NanArgs &args) : val(cbWrapper<ReturnType>(args[Index].template As<v8::Function>())) {} \
                                                    \
	template <typename NanArgs>                     \
	inline ArgType get(const NanArgs &args) {       \
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
