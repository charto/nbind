// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

enum class WrapperFlags : uint32_t {
	none = 0,
	constant = 1
};

inline WrapperFlags operator& (WrapperFlags a, WrapperFlags b) {
	return(static_cast<WrapperFlags>(
		static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
	));
}

inline bool operator! (WrapperFlags f) { return(f == WrapperFlags::none); }

// Nullable policy

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

// Strict policy

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

// Policy list

template <typename...>
struct PolicyListType {};

template <typename PolicyList>
struct PolicyLister {};

template <typename... Policies>
struct PolicyLister<PolicyListType<Policies...>> {
	static const char **getNameList() {
		static const char *nameList[] = { Policies::getName()..., nullptr };
		return(nameList);
	}
};

// Policy autodetection part 1

struct NoPolicy {
	template <typename ArgType, typename Transformed>
	struct Transform {
		typedef Transformed Type;
	};

	static const char *getName() {
		return(nullptr);
	}
};

template <typename ArgType>
NoPolicy detectPolicies(ArgType, int);

// Value object policy (autodetected)

template<typename ArgType>
struct ValueType {};

struct ValuePolicy {
	template <typename ArgType, typename Transformed>
	struct Transform {
		typedef ValueType<Transformed> Type;
	};

	static const char *getName() {
		static const char *name = "Value";
		return(name);
	}
};

class cbOutput;

// SFINAE test if the class has a toJS method with cbOutput parameter.

template <typename ArgType, typename = decltype(std::declval<ArgType>().toJS(std::declval<cbOutput>()))>
ValuePolicy detectPolicies(ArgType, double);

// Policy autodetection part 2

template <typename ArgType>
struct DetectPolicies {
	typedef decltype(detectPolicies(std::declval<ArgType>(), 0.0)) Policy;

	typedef typename Policy::template Transform<ArgType, ArgType>::Type Type;

	static const char **getPolicies() {
		return(PolicyLister<PolicyListType<Policy>>::getNameList());
	}
};

template<>
struct DetectPolicies<void> {
	typedef void Type;
};

// Policy execution

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

} // namespace
