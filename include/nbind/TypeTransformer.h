// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template <typename ArgType> struct BindingType;

template<typename ArgType, typename PolicyList>
struct TypeTransformer {
	typedef BindingType<typename ExecutePolicies<PolicyList>::template Transformed<
		typename DetectPolicies<ArgType>::Type
	>::Type> Binding;

	typedef typename Binding::Type Type;
};

} // namespace
