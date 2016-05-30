// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

extern "C" {
	extern unsigned int _nbind_register_callback_signature(const TYPEID *types, unsigned int typeCount);
}

class BaseCallbackSignature {

public:

	BaseCallbackSignature(const TYPEID *typeList, unsigned int arity) :
		typeList(typeList),
		arity(arity),
		num(_nbind_register_callback_signature(
			typeList,
			arity + 1
		)) {}

	const TYPEID *getTypeList() const { return(typeList); }
	unsigned int getArity() const { return(arity); }
	unsigned int getNum() const { return(num); }

private:

	const TYPEID *typeList;
	const unsigned int arity;
	const unsigned int num;

};

template <typename ReturnType, typename... Args>
class CallbackSignature : public BaseCallbackSignature {

public:

	typedef ReturnType(*MethodType)(Args...);

	CallbackSignature() : BaseCallbackSignature(
		listTypes<ReturnType, Args...>(),
		sizeof...(Args)
	) {}

	// Making the instance a direct class member fails on some platforms.
	// Maybe it registers the signature too early.

	static CallbackSignature &getInstance() {
		// Linkage for a singleton instance of each templated class.
		static CallbackSignature instance;
		return(instance);
	}

};

} // namespace
