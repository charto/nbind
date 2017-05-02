// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class Int64 {};

template <typename ArgType>
static ArgType int64FromWire(WireType arg, void(init)(const Nan::FunctionCallbackInfo<v8::Value> &args)) {
	Nan::HandleScope();

	auto target = arg->ToObject();
	auto fromJS = target->Get(Nan::New<v8::String>("fromJS").ToLocalChecked());

	if(!fromJS->IsFunction()) throw(std::runtime_error("Type mismatch"));

	ArgType storage = 0;

	// TODO: cache this for a speedup (replace init param).
	auto storageTemplate = Nan::New<v8::ObjectTemplate>();
	storageTemplate->SetInternalFieldCount(1);
	Nan::SetCallAsFunctionHandler(storageTemplate, init);

	auto instance = Nan::NewInstance(storageTemplate).ToLocalChecked();
	Nan::SetInternalFieldPointer(instance, 0, &storage);

	// TODO: cache this for a speedup.
	cbFunction converter(v8::Local<v8::Function>::Cast(fromJS));
	converter.callMethod<void>(target, instance);

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
		ArgType &storage = *static_cast<ArgType *>(Nan::GetInternalFieldPointer(args.Holder(), 0));

		unsigned int argc = args.Length();
		if(argc > 0) storage = args[0]->Uint32Value();

		args.GetReturnValue().Set(Nan::Undefined());
	}

	template <typename ArgType>
	static void int64Init(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		ArgType &storage = *static_cast<ArgType *>(Nan::GetInternalFieldPointer(args.Holder(), 0));

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
		ArgType &storage = *static_cast<ArgType *>(Nan::GetInternalFieldPointer(args.Holder(), 0));

		unsigned int argc = args.Length();
		if(argc > 0) storage = args[0]->Uint32Value();
		if(argc > 1) storage += static_cast<uint64_t>(args[1]->Uint32Value()) << 32;

		args.GetReturnValue().Set(Nan::Undefined());
	}

	template <typename ArgType>
	static void int64Init(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		ArgType &storage = *static_cast<ArgType *>(Nan::GetInternalFieldPointer(args.Holder(), 0));

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
