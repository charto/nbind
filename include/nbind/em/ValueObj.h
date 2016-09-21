// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles value objects, which are represented by equivalent C++ and
// JavaScript classes, with toJS and fromJS methods calling each others'
// constructors to marshal the class between languages and providing a similar
// API in both.

#pragma once

namespace nbind {

extern "C" {
	extern unsigned int _nbind_get_value_object(unsigned int index, ArgStorage *storage);
}

template<> struct BindingType<cbOutput::CreateValue> {

	typedef int Type;

};

template <typename ArgType>
inline auto BindingType<ValueType<ArgType>>::toWireType(ArgType &&arg) -> WireType {
	cbFunction *jsConstructor = BindClass<
		typename std::remove_const<ObjType>::type
	>::getInstance().getValueConstructorJS();

	if(jsConstructor != nullptr) {
		cbOutput construct(*jsConstructor);

		arg.toJS(construct);

		return(reinterpret_cast<WireType>(construct.getSlot()));
	} else {
		// Value type JavaScript class is missing or not registered.
		return(BindingType<ObjType>::toWireType(std::forward<ArgType>(arg)));
	}
}

template <typename ArgType>
inline ArgType BindingType<ValueType<ArgType>>::fromWireType(WireType arg) {
	uintptr_t index = reinterpret_cast<int>(arg);
	if(index & 1) {
		// Constructor argument is an unused dummy value.
		TemplatedArgStorage<ArgType> storage(0);

		_nbind_get_value_object(index, &storage);

		return(storage.getBound());
	} else return(*reinterpret_cast<Type *>(arg));
}

} // namespace
