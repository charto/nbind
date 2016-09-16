// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles value objects, which are represented by equivalent C++ and
// JavaScript classes, with toJS and fromJS methods calling each others'
// constructors to marshal the class between languages and providing a similar
// API in both.

#pragma once

namespace nbind {

template <typename BaseType, typename ArgType>
static inline WireType makeExternal(ArgType *arg, TypeFlags flags) {
	if(std::is_const<ArgType>::value) flags = flags | TypeFlags::isConst;

#ifndef DUPLICATE_POINTERS

	auto ref = BindWrapper<BaseType>::findInstance(arg, flags);

	if(!ref->IsEmpty()) {
		return(Nan::New<v8::Object>(*ref));
	}

#endif // DUPLICATE_POINTERS

	unsigned int constructorNum = BindClass<BaseType>::getInstance().wrapperConstructorNum;
	v8::Local<v8::Function> constructor = Overloader::getDef(constructorNum).constructorJS->GetFunction();

	// TODO: first argument should be a unique marker of some kind.

	const unsigned int argc = 2;
	v8::Local<v8::Value> argv[] = {
		((flags & TypeFlags::refMask) == TypeFlags::isSharedPtr ?
			Nan::New<v8::External>(new std::shared_ptr<ArgType>(arg)) :
			Nan::New<v8::External>(const_cast<BaseType *>(arg))
		),
		Nan::New<v8::Uint32>(static_cast<uint32_t>(flags))
	};

	// This will try to call the C++ constructor, so Overloader or Creator
	// needs to detect the argument is a v8::External and just wrap it instead.

	return(constructor->NewInstance(argc, argv));
}

template <typename ArgType>
inline WireType BindingType<ArgType *>::toWireType(ArgType *arg) {
	if(arg == nullptr) return(Nan::Null());

	return(makeExternal<BaseType>(arg, TypeFlags::none));
}

template <typename ArgType>
inline WireType BindingType<std::shared_ptr<ArgType>>::toWireType(
	std::shared_ptr<ArgType> arg
) {
	if(arg == nullptr || !arg.use_count()) return(Nan::Null());

	return(makeExternal<BaseType>(arg.get(), TypeFlags::isSharedPtr));
}

template <> struct BindingType<v8::Local<v8::Function>> {

	static inline WireType toWireType(v8::Local<v8::Function> arg) {
		return(arg);
	}

};

template <typename ArgType>
inline ArgType BindingType<ValueType<ArgType>>::fromWireType(WireType arg) noexcept(false) {
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
