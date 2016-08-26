// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template <typename ArgType>
struct NullableType {};

struct Nullable {
	template <typename ArgType, typename Transformed>
	struct Transform {
		typedef Transformed Type;
	};

	template<typename ArgType, typename Transformed>
	struct Transform<ArgType *, Transformed> {
		typedef NullableType<Transformed> Type;
	};

	static const char *getName() {
		static const char *name = "Nullable";
		return(name);
	}
};

#define DEFINE_STRICT_BINDING_TYPE(ArgType) \
template<typename Transformed>              \
struct Transform<ArgType, Transformed> {    \
	typedef StrictType<Transformed> Type;   \
}

template <typename ArgType>
struct StrictType {};

struct Strict {
	template <typename ArgType, typename Transformed>
	struct Transform {
		typedef Transformed Type;
	};

	DEFINE_STRICT_BINDING_TYPE(bool);

	DEFINE_STRICT_BINDING_TYPE(double);
	DEFINE_STRICT_BINDING_TYPE(float);

	DEFINE_STRICT_BINDING_TYPE(unsigned int);
	DEFINE_STRICT_BINDING_TYPE(unsigned short);
	DEFINE_STRICT_BINDING_TYPE(unsigned char);

	DEFINE_STRICT_BINDING_TYPE(signed int);
	DEFINE_STRICT_BINDING_TYPE(signed short);
	DEFINE_STRICT_BINDING_TYPE(signed char);

	DEFINE_STRICT_BINDING_TYPE(char);

	DEFINE_STRICT_BINDING_TYPE(unsigned char *);
	DEFINE_STRICT_BINDING_TYPE(char *);
	DEFINE_STRICT_BINDING_TYPE(const unsigned char *);
	DEFINE_STRICT_BINDING_TYPE(const char *);

	DEFINE_STRICT_BINDING_TYPE(std::string);

	static const char *getName() {
		static const char *name = "Strict";
		return(name);
	}
};

template <typename...>
struct PolicyListType {};

template<typename... Policies>
struct ExecutePolicies;

template<>
struct ExecutePolicies<PolicyListType<>> {
	template<typename ArgType>
	struct Transformed {
		typedef ArgType Type;
	};
};

template<typename Policy, typename... Remaining>
struct ExecutePolicies<PolicyListType<Policy, Remaining...>> {
	template<typename ArgType>
	struct Transformed {
		typedef typename Policy::template Transform<
			ArgType,
			typename ExecutePolicies<PolicyListType<Remaining...>>::template Transformed<ArgType>::Type
		>::Type Type;
	};
};

template <typename Signature>
struct Overloaded {};

} // namespace
