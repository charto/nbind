// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

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

		// Success.
		return(1);
	} else {
		// Failure: value type JavaScript class is missing or not registered.
		return(0);
	}
}

// TODO

template <typename ArgType>
ArgType BindingType<ArgType>::fromWireType(WireType arg) {
	ArgType data;

	return(data);
}

} // namespace
