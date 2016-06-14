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
		Policies...
	) {
		typedef FunctionSignature<decltype(func), std::nullptr_t, ReturnType, Args...> Signature;

		registerFunction(
			name,
			Signature::getDirect(func),
			Signature::addMethod(func),
			&Signature::getInstance()
		);
	}
};

} // namespace
