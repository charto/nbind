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

} // namespace
