// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles value objects, which are represented by equivalent C++ and
// JavaScript classes, with toJS and fromJS methods calling each others'
// constructors to marshal the class between languages and providing a similar
// API in both.

#pragma once

namespace nbind {

extern "C" {
	extern unsigned int _nbind_get_value_object(unsigned int index, ArgStorage &storage);
}

template<> struct BindingType<cbOutput::CreateValue> {

	typedef int Type;

};

/*
TODO:

template <typename ArgType>
inline uint32_t BindingType<ArgType *>::toWireType(ArgType *arg) {
	return(0);
}
*/

template <typename ArgType>
inline int BindingType<ArgType>::toWireType(ArgType arg) {
	cbFunction *jsConstructor = BindClass<ArgType>::getInstance().getValueConstructorJS();

	if(jsConstructor != nullptr) {
		cbOutput construct(*jsConstructor);

		arg.toJS(construct);

		return(construct.getSlot());
	} else {
		// Value type JavaScript class is missing or not registered.
		return(0);
	}
}

class Int64 {};

template <>
inline int BindingType<uint64_t>::toWireType(uint64_t arg) {
	cbFunction *jsConstructor = BindClass<Int64>::getInstance().getValueConstructorJS();

	if(jsConstructor != nullptr) {
		cbOutput construct(*jsConstructor);

		return(construct(uint32_t(arg >> 32), uint32_t(arg), false));
	} else {
		// Int64 JavaScript class is missing or not registered.
		return(0);
	}
}

template <>
inline int BindingType<int64_t>::toWireType(int64_t arg) {
	cbFunction *jsConstructor = BindClass<Int64>::getInstance().getValueConstructorJS();

	if(jsConstructor != nullptr) {
		cbOutput construct(*jsConstructor);

		bool sign = arg < 0;
		if(sign) arg = -arg;

		return(construct(uint32_t(arg >> 32), uint32_t(arg), sign));
	} else {
		// Int64 JavaScript class is missing or not registered.
		return(0);
	}
}

template <typename ArgType>
ArgType BindingType<ArgType>::fromWireType(int index) {
	// Argument is an unused dummy value.

	TemplatedArgStorage<ArgType> storage(0);

	_nbind_get_value_object(index, storage);

	return(storage.getBound());
}

} // namespace
