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
	if(arg == nullptr) return(Nan::Null());

	WrapperFlags flags = std::is_const<ArgType>::value ?
		WrapperFlags::constant :
		WrapperFlags::none;

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
		Nan::New<v8::External>(const_cast<BaseType *>(arg)),
		Nan::New<v8::Uint32>(static_cast<uint32_t>(flags))
	};

	// This will try to call the C++ constructor, so Overloader or Creator
	// needs to detect the argument is a v8::External and just wrap it instead.

	return(constructor->NewInstance(argc, argv));
}

template <typename ArgType>
inline WireType BindingType<ArgType &>::toWireType(ArgType &arg) {
	// TODO: Somehow prevent using the object as a non-const argument later!

	return(BindingType<ArgType *>::toWireType(&arg));
}

template <> struct BindingType<v8::Local<v8::Function>> {

	static inline WireType toWireType(v8::Local<v8::Function> arg) {
		return(arg);
	}

};

template <typename ArgType>
inline ArgType convertFromWire(WireType arg) noexcept(false) {
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

class Int64 {};

template <typename ArgType>
static ArgType int64FromWire(WireType arg, void(init)(const Nan::FunctionCallbackInfo<v8::Value> &args)) {
	Nan::HandleScope();

	auto target = arg->ToObject();
	auto fromJS = target->Get(Nan::New<v8::String>("fromJS").ToLocalChecked());

	if(!fromJS->IsFunction()) throw(std::runtime_error("Type mismatch"));

	ArgType storage = 0;

	// TODO: cache this for a speedup.
	cbFunction converter(v8::Local<v8::Function>::Cast(fromJS));

	v8::Local<v8::FunctionTemplate> constructorTemplate = Nan::New<v8::FunctionTemplate>(
		init,
		Nan::New<v8::External>(&storage)
	);

	// TODO: cache this for a speedup.
	auto constructor = constructorTemplate->GetFunction();

	converter.callMethod<void>(target, constructor);

	return(storage);
}

// Fast code path for 32-bit types.

template <int size> struct Int64Converter {
	template <typename ArgType>
	static inline WireType uint64ToWire(ArgType arg) {
		return(Nan::New<v8::Uint32>(static_cast<uint32_t>(arg)));
	}

	template <typename ArgType>
	static inline WireType int64ToWire(ArgType arg) {
		return(Nan::New<v8::Int32>(static_cast<int32_t>(arg)));
	}

	template <typename ArgType>
	static void uint64Init(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		ArgType &storage = *static_cast<ArgType *>(v8::Handle<v8::External>::Cast(args.Data())->Value());

		unsigned int argc = args.Length();
		if(argc > 0) storage = args[0]->Uint32Value();

		args.GetReturnValue().Set(Nan::Undefined());
	}

	template <typename ArgType>
	static void int64Init(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		ArgType &storage = *static_cast<ArgType *>(v8::Handle<v8::External>::Cast(args.Data())->Value());

		unsigned int argc = args.Length();
		if(argc > 0) storage = args[0]->Uint32Value();
		if(argc > 2 && args[2]->BooleanValue()) storage = -storage;

		args.GetReturnValue().Set(Nan::Undefined());
	}
};

// Handle 64-bit types.

template<> struct Int64Converter<8> {
	template <typename ArgType>
	static inline WireType uint64ToWire(ArgType arg) {
		if(arg <= 0xffffffffU) {
			// Number fits in uint32_t.
			return(Nan::New<v8::Uint32>(static_cast<uint32_t>(arg)));
		} else if(arg <= 0x20000000000000ULL) {
			// Number fits in double.
			return(Nan::New<v8::Number>(static_cast<double>(arg)));
		}

		cbFunction *jsConstructor = BindClass<Int64>::getInstance().getValueConstructorJS();

		if(jsConstructor != nullptr) {
			// Construct custom bignum object from high and low 32-bit halves.
			v8::Local<v8::Value> output = Nan::Undefined();
			cbOutput construct(*jsConstructor, &output);

			construct(
				static_cast<uint32_t>(arg),
				static_cast<uint32_t>(arg >> 32),
				false
			);

			return(output);
		} else {
			// Int64 JavaScript class is missing or not registered.
			// Just cast to double then.
			return(Nan::New<v8::Number>(static_cast<double>(arg)));
		}
	}

	template <typename ArgType>
	static inline WireType int64ToWire(ArgType arg) {
		if(arg >= -0x80000000 && arg <= 0x7fffffff) {
			return(Nan::New<v8::Int32>(static_cast<int32_t>(arg)));
		} else if(arg >= -0x20000000000000LL && arg <= 0x20000000000000LL) {
			return(Nan::New<v8::Number>(static_cast<double>(arg)));
		}

		cbFunction *jsConstructor = BindClass<Int64>::getInstance().getValueConstructorJS();

		if(jsConstructor != nullptr) {
			v8::Local<v8::Value> output = Nan::Undefined();
			cbOutput construct(*jsConstructor, &output);

			bool sign = arg < 0;
			if(sign) arg = -arg;

			construct(
				static_cast<uint32_t>(arg),
				static_cast<uint32_t>(static_cast<uint64_t>(arg) >> 32),
				sign
			);

			return(output);
		} else {
			// Int64 JavaScript class is missing or not registered.
			return(Nan::New<v8::Number>(static_cast<double>(arg)));
		}
	}

	template <typename ArgType>
	static void uint64Init(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		ArgType &storage = *static_cast<ArgType *>(v8::Handle<v8::External>::Cast(args.Data())->Value());

		unsigned int argc = args.Length();
		if(argc > 0) storage = args[0]->Uint32Value();
		if(argc > 1) storage += static_cast<uint64_t>(args[1]->Uint32Value()) << 32;

		args.GetReturnValue().Set(Nan::Undefined());
	}

	template <typename ArgType>
	static void int64Init(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		ArgType &storage = *static_cast<ArgType *>(v8::Handle<v8::External>::Cast(args.Data())->Value());

		unsigned int argc = args.Length();
		if(argc > 0) storage = args[0]->Uint32Value();
		if(argc > 1) storage += static_cast<uint64_t>(args[1]->Uint32Value()) << 32;
		if(argc > 2 && args[2]->BooleanValue()) storage = -storage;

		args.GetReturnValue().Set(Nan::Undefined());
	}
};

#define DEFINE_INT64_BINDING_TYPE(ArgType, encode, decode)  \
template <> struct BindingType<ArgType> {                   \
	typedef ArgType Type;                                   \
	                                                        \
	static inline bool checkType(WireType arg) {            \
		return(true);                                       \
	}                                                       \
	                                                        \
	static inline WireType toWireType(Type arg) {           \
		return(Int64Converter<sizeof(Type)>::encode(arg));  \
	}                                                       \
	                                                        \
	static inline Type fromWireType(WireType arg) {         \
		if(arg->IsObject()) {                               \
			return(int64FromWire<ArgType>(arg, Int64Converter<sizeof(Type)>::decode<ArgType>)); \
		} else {                                            \
			return(static_cast<Type>(arg->NumberValue()));  \
		}                                                   \
	}                                                       \
};                                                          \
                                                            \
template <> struct BindingType<StrictType<ArgType>> : public BindingType<ArgType> { \
	static inline bool checkType(WireType arg) {            \
		/* TODO: check type of object (bound to Int64?) */  \
		return(arg->IsNumber() || arg->IsObject());         \
	}                                                       \
}

// Handle possibly 64-bit types.
// Types detected to fit in 32 bits automatically use a faster code path.

DEFINE_INT64_BINDING_TYPE(unsigned long, uint64ToWire, uint64Init);
DEFINE_INT64_BINDING_TYPE(unsigned long long, uint64ToWire, uint64Init);
DEFINE_INT64_BINDING_TYPE(signed long, int64ToWire, int64Init);
DEFINE_INT64_BINDING_TYPE(signed long long, int64ToWire, int64Init);

} // namespace
