// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

template <typename ArgType> struct BindingType;

template<typename ArgType, typename PolicyList = PolicyListType<>>
struct TypeTransformer {
	typedef BindingType<typename ExecutePolicies<PolicyList>::template Transformed<
		typename DetectPolicies<ArgType>::Type
	>::Type> Binding;

#	if defined(BUILDING_NODE_EXTENSION)
		typedef v8::Local<v8::Value> WireType;
#	else
		typedef typename Binding::WireType WireType;
#	endif

	typedef typename Binding::Type Type;
};

template <typename ArgType, typename WireType>
inline typename TypeTransformer<ArgType>::Type convertFromWire(WireType arg) {
	return(TypeTransformer<ArgType>::Binding::fromWireType(arg));
}

// Convert any C++ type to the corresponding JavaScript type.
// Call correct type converter using perfect forwarding (moving doesn't work).

template <typename ReturnType>
inline typename TypeTransformer<ReturnType>::WireType convertToWire(ReturnType result) {
	return(TypeTransformer<ReturnType>::Binding::toWireType(std::forward<ReturnType>(result)));
}

} // namespace
