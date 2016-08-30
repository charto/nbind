// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class FunctionDefiner {

public:

	template <typename ReturnType, typename... Args, typename... Policies>
	FunctionDefiner(
		const char* name,
		ReturnType(*func)(Args...),
		Policies... policies
	) {
		typedef FunctionSignature<
			decltype(func),
			std::nullptr_t,
			typename SkipNamePolicy<PolicyListType<Policies...>>::Type,
			ReturnType,
			Args...
		> Signature;

		registerFunction(
			executeNamePolicy(name, policies...),
			Signature::getDirect(func),
			Signature::addMethod(func, WrapperFlags::none),
			&Signature::getInstance()
		);
	}
};

} // namespace
