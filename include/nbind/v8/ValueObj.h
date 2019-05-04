// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles value objects, which are represented by equivalent C++ and
// JavaScript classes, with toJS and fromJS methods calling each others'
// constructors to marshal the class between languages and providing a similar
// API in both.

#pragma once

namespace nbind {

template <typename BaseType, typename ArgType>
struct ExternalPtr;

template <typename BaseType, typename ArgType>
struct ExternalPtr<BaseType, ArgType *> {
	static void *make(ArgType *arg) {
		return(const_cast<BaseType *>(arg));
	}
};

template <typename BaseType, typename ArgType>
struct ExternalPtr<BaseType, std::shared_ptr<ArgType>> {
	static void *make(std::shared_ptr<ArgType> &&ptr) {
		return(new std::shared_ptr<ArgType>(std::move(ptr)));
	}
};

template <typename BaseType, typename ArgType>
struct ExternalPtr<BaseType, std::unique_ptr<ArgType>> {
	static void *make(std::shared_ptr<ArgType> &&ptr) {
		return(new std::shared_ptr<ArgType>(std::move(ptr)));
	}
};

template <typename BaseType, typename TargetType, typename ArgType>
static inline WireType makeExternal(TypeFlags flags, TargetType *ptr, ArgType &&arg) {
	if(std::is_const<TargetType>::value) flags = flags | TypeFlags::isConst;

#ifndef NBIND_DUPLICATE_POINTERS

	auto ref = BindWrapper<BaseType>::findInstance(ptr, flags);

	if(!ref->IsEmpty()) {
		return(Nan::New<v8::Object>(*ref));
	}

#endif // NBIND_DUPLICATE_POINTERS

	unsigned int constructorNum = BindClass<BaseType>::getInstance().wrapperConstructorNum;
	Nan::Callback *constructorJS = Overloader::getDef(constructorNum).constructorJS;

	if(constructorJS == nullptr) {
		Nan::ThrowError("Unbound type");
		return(Nan::Undefined());
	}

	v8::Local<v8::Function> constructor = constructorJS->GetFunction();

	// TODO: first argument should be a unique marker of some kind.

	const unsigned int argc = 2;
	v8::Local<v8::Value> argv[] = {
		Nan::New<v8::External>(ExternalPtr<BaseType, ArgType>::make(std::move(arg))),
		Nan::New<v8::Uint32>(static_cast<uint32_t>(flags))
	};

	// This will try to call the C++ constructor, so Overloader or Creator
	// needs to detect the argument is a v8::External and just wrap it instead.

	return(Nan::NewInstance(constructor, argc, argv).ToLocalChecked());
}

template <typename ArgType>
inline WireType BindingType<ArgType *>::toWireType(ArgType *arg) {
	if(arg == nullptr) return(Nan::Null());

	return(makeExternal<BaseType>(TypeFlags::none, arg, std::move(arg)));
}

template <typename ArgType>
inline WireType BindingType<std::shared_ptr<ArgType>>::toWireType(
	std::shared_ptr<ArgType> &&arg
) {
	if(arg == nullptr || !arg.use_count()) return(Nan::Null());

	return(makeExternal<BaseType>(TypeFlags::isSharedPtr, arg.get(), std::move(arg)));
}

template <typename ArgType>
inline WireType BindingType<std::unique_ptr<ArgType>>::toWireType(
	std::unique_ptr<ArgType> &&arg
) {
	if(arg == nullptr) return(Nan::Null());

	return(makeExternal<BaseType>(TypeFlags::isSharedPtr, arg.get(), std::move(arg)));
}

// Allow passing internal wrapped value object storage pointers.

template <> struct BindingType<v8::Local<v8::Object>> {

	typedef v8::Local<v8::Object> Type;

	static inline WireType toWireType(v8::Local<v8::Object> arg) {
		return(arg);
	}

};

template <typename ArgType>
inline ArgType BindingType<ValueType<ArgType>>::fromWireType(WireType arg) noexcept(false) {
	auto target = Nan::To<v8::Object>(arg).ToLocalChecked();
	auto fromJS = target->Get(Nan::New<v8::String>("fromJS").ToLocalChecked());

	if(!fromJS->IsFunction()) throw(std::runtime_error("Type mismatch"));

	BindClassBase &bindClass = BindClass<ArgType>::getInstance();

	TemplatedArgStorage<ArgType> storage(bindClass.valueConstructorNum);

	auto instance = Nan::NewInstance(Nan::New(bindClass.storageTemplate)).ToLocalChecked();
	// Data specifically for createValue function.
	Nan::SetInternalFieldPointer(instance, 0, &storage);

	// TODO: cache this for a speedup.
	cbFunction converter(v8::Local<v8::Function>::Cast(fromJS));
	converter.callMethod<void>(target, instance);

	const char *message = Status::getError();
	if(message) throw(std::runtime_error(message));

	return(storage.getBound());
}

} // namespace
