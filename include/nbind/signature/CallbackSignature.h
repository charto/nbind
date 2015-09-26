// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

extern "C" {
	extern unsigned int _nbind_register_callback_signature(const TYPEID *types, unsigned int typeCount);
}

class BaseCallbackSignature {

public:

	BaseCallbackSignature(const TYPEID *typeList, unsigned int arity) : typeList(typeList), arity(arity) {
		num = _nbind_register_callback_signature(
			typeList,
			arity
		);
	}

	const TYPEID *getTypeList() { return(typeList); }
	unsigned int getArity() { return(arity); }
	unsigned int getNum() { return(num); }

private:

	const TYPEID *typeList;
	unsigned int arity;
	unsigned int num;

};

template <typename ReturnType, typename... Args>
class CallbackSignature : public BaseCallbackSignature {

public:

	CallbackSignature() : BaseCallbackSignature(
		listTypes<ReturnType, Args...>(),
		sizeof...(Args)
	) {}

	typedef ReturnType(*MethodType)(Args...);

};

} // namespace
