// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles value objects, which are represented by equivalent C++ and
// JavaScript classes, with toJS and fromJS methods calling each others'
// constructors to marshal the class between languages and providing a similar
// API in both.

#pragma once

namespace nbind {

template <typename ArgType>
inline WireType BindingType<ArgType *>::toWireType(ArgType *arg) {
	if(arg == nullptr) return(Nan::Undefined());

#ifndef DUPLICATE_POINTERS

	auto ref = BindWrapper<ArgType>::findInstance(arg);

	if(!ref->IsEmpty()) {
		return(Nan::New<v8::Object>(*ref));
	}

#endif // DUPLICATE_POINTERS

	unsigned int constructorNum = BindClass<ArgType>::getInstance().wrapperConstructorNum;
	v8::Local<v8::Function> constructor = Overloader::getDef(constructorNum).constructorJS->GetFunction();

	const unsigned int argc = 1;
	v8::Local<v8::Value> argv = Nan::New<v8::External>(arg);

	return(constructor->NewInstance(argc, &argv));
}

template <> struct BindingType<v8::Local<v8::Function>> {

	static inline WireType toWireType(v8::Local<v8::Function> arg) {
		return(arg);
	}

};

// Call the toJS method of a returned C++ object, to convert it into a JavaScript object.
// This is used when a C++ function is called from JavaScript.
// A functor capable of calling the correct JavaScript constructor is passed to toJS,
// which must call the functor with arguments in the correct order.
// The functor calls the JavaScript constructor and writes a pointer to the resulting object
// directly into a local handle called "output" which is returned to JavaScript.

template <typename ArgType>
inline WireType BindingType<ArgType>::toWireType(ArgType arg) {
	v8::Local<v8::Value> output = Nan::Undefined();
	cbFunction *jsConstructor = BindClass<ArgType>::getInstance().getValueConstructorJS();

	if(jsConstructor != nullptr) {
		cbOutput construct(*jsConstructor, &output);

		arg.toJS(construct);
	} else {
		throw(std::runtime_error("Value type JavaScript class is missing or not registered"));
	}

	return(output);
}

template <typename ArgType>
ArgType BindingType<ArgType>::fromWireType(WireType arg) noexcept(false) {
	Nan::HandleScope();

	auto target = arg->ToObject();
	auto fromJS = target->Get(Nan::New<v8::String>("fromJS").ToLocalChecked());

	if(!fromJS->IsFunction()) throw(std::runtime_error("Type mismatch"));

	TemplatedArgStorage<ArgType> storage(
		BindClass<ArgType>::getInstance().valueConstructorNum
	);

	// TODO: cache this for a speedup.
	cbFunction converter(v8::Local<v8::Function>::Cast(fromJS));

	v8::Local<v8::FunctionTemplate> constructorTemplate = Nan::New<v8::FunctionTemplate>(
		Overloader::createValue,
		// Data specifically for createValue function.
		Nan::New<v8::External>(&storage)
	);

	// TODO: cache this for a speedup.
	auto constructor = constructorTemplate->GetFunction();

	converter.callMethod<void>(target, constructor);

	const char *message = Status::getError();
	if(message) throw(std::runtime_error(message));

	return(storage.getBound());
}

} // namespace
