// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// This is effectively an enum with members accessible using dot notation.
// It allows copy-pasting the later TypeFlags definition between
// C++ and TypeScript using identical syntax.

struct TypeFlagBaseType {
	constexpr TypeFlagBaseType() {}

	// GCC bug workaround.
	// Constexpr struct instance members cannot refer to each other.

	struct inner {
		static constexpr uint32_t flag = 1;
		static constexpr uint32_t num = flag * 8;
		static constexpr uint32_t ref = num * 16;
		static constexpr uint32_t kind = ref * 8;
	};

	uint32_t flag = inner::flag;
	uint32_t ref = inner::ref;
	uint32_t kind = inner::kind;
	uint32_t num = inner::num;
};

// Visual Studio bug workaround. The struct must be defined separately.

constexpr struct TypeFlagBaseType TypeFlagBase;

// These must match Type.ts.

enum class TypeFlags : uint32_t {
	none = 0,

	flagMask = TypeFlagBase.flag * 7,
	isConst = TypeFlagBase.flag * 1,
	isValueObject = TypeFlagBase.flag * 2,
	isMethod = TypeFlagBase.flag * 4,

	numMask = TypeFlagBase.num * 15,
	isUnsigned = TypeFlagBase.num * 1,
	isSignless = TypeFlagBase.num * 2,
	isFloat = TypeFlagBase.num * 4,
	isBig = TypeFlagBase.num * 8,

	refMask = TypeFlagBase.ref * 7,
	isPointer = TypeFlagBase.ref * 1,
	isReference = TypeFlagBase.ref * 2,
	isRvalueRef = TypeFlagBase.ref * 3,
	isSharedPtr = TypeFlagBase.ref * 4,
	isUniquePtr = TypeFlagBase.ref * 5,

	kindMask = TypeFlagBase.kind * 15,
	isArithmetic = TypeFlagBase.kind * 1,
	isClass = TypeFlagBase.kind * 2,
	isClassPtr = TypeFlagBase.kind * 3,
	isSharedClassPtr = TypeFlagBase.kind * 4,
	isVector = TypeFlagBase.kind * 5,
	isArray = TypeFlagBase.kind * 6,
	isCString = TypeFlagBase.kind * 7,
	isString = TypeFlagBase.kind * 8,
	isCallback = TypeFlagBase.kind * 9,
	isOther = TypeFlagBase.kind * 10
};

inline TypeFlags operator& (TypeFlags a, TypeFlags b) {
	return(static_cast<TypeFlags>(
		static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
	));
}
inline TypeFlags operator| (TypeFlags a, TypeFlags b) {
	return(static_cast<TypeFlags>(
		static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
	));
}

inline bool operator! (TypeFlags f) { return(f == TypeFlags::none); }

inline TypeFlags operator~ (TypeFlags f) {
	return(static_cast<TypeFlags>(
		~static_cast<uint32_t>(f)
	));
}

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
NoPolicy detectPolicies(const ArgType &, int);

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
ValuePolicy detectPolicies(const ArgType &, double);

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
