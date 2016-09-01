// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

extern "C" {
	extern unsigned int _nbind_get_int_64(unsigned int index, uint32_t *storage);
}

class Int64 {};

// 2^64, first integer not representable with uint64_t.
// Start of range used for other flags.
const double valueBase = 18446744073709551616.0;

template <int size> struct Int64Converter {
	typedef int WireType;

	template <typename ArgType>
	static inline WireType uint64ToWire(ArgType arg) {
		return(static_cast<int>(arg));
	}

	template <typename ArgType>
	static inline WireType int64ToWire(ArgType arg) {
		return(static_cast<int>(arg));
	}

	template <typename ArgType>
	static inline ArgType fromWire(WireType arg) {
		return(arg);
	}
};

template<> struct Int64Converter<8> {
	typedef double WireType;

	template <typename ArgType>
	static inline WireType uint64ToWire(ArgType arg) {
		if(arg <= 0x20000000000000ULL) {
			// Number fits in double.
			return(static_cast<double>(arg));
		}

		cbFunction *jsConstructor = BindClass<Int64>::getInstance().getValueConstructorJS();

		if(jsConstructor != nullptr) {
			// Construct custom bignum object from high and low 32-bit halves.
			cbOutput construct(*jsConstructor);

			return(
				construct(
					static_cast<uint32_t>(arg),
					static_cast<uint32_t>(arg >> 32),
					false
				) * 4096 + valueBase
			);
		} else {
			// Int64 JavaScript class is missing or not registered.
			// Just cast to double then.
			return(static_cast<double>(arg));
		}
	}

	template <typename ArgType>
	static inline WireType int64ToWire(ArgType arg) {
		if(arg >= -0x20000000000000LL && arg <= 0x20000000000000LL) {
			return(static_cast<double>(arg));
		}

		cbFunction *jsConstructor = BindClass<Int64>::getInstance().getValueConstructorJS();

		if(jsConstructor != nullptr) {
			cbOutput construct(*jsConstructor);

			bool sign = arg < 0;
			if(sign) arg = -arg;

			return(
				construct(
					static_cast<uint32_t>(arg),
					static_cast<uint32_t>(static_cast<uint64_t>(arg) >> 32),
					sign
				) * 4096 + valueBase
			);
		} else {
			// Int64 JavaScript class is missing or not registered.
			return(static_cast<double>(arg));
		}
	}

	template <typename ArgType>
	static inline ArgType fromWire(WireType arg) {
		if(arg <= valueBase) return(arg);

		unsigned int index = (arg - valueBase) / 4096;
		ArgType storage = 0;

		_nbind_get_int_64(index, reinterpret_cast<uint32_t *>(&storage));

		return(storage);
	}
};

#define DEFINE_INT64_BINDING_TYPE(ArgType, encode)          \
template <> struct BindingType<ArgType> {                   \
	typedef ArgType Type;                                   \
	typedef typename Int64Converter<sizeof(Type)>::WireType WireType; \
	                                                        \
	static inline WireType toWireType(Type arg) {           \
		return(Int64Converter<sizeof(Type)>::encode(arg));  \
	}                                                       \
	                                                        \
	static inline Type fromWireType(WireType arg) {         \
		return(Int64Converter<sizeof(Type)>::fromWire<Type>(arg)); \
	}                                                       \
};                                                          \
                                                            \
template <> struct BindingType<StrictType<ArgType>> : public BindingType<ArgType> {}

// Handle possibly 64-bit types.
// Types detected to fit in 32 bits automatically use a faster code path.

DEFINE_INT64_BINDING_TYPE(unsigned long, uint64ToWire);
DEFINE_INT64_BINDING_TYPE(unsigned long long, uint64ToWire);
DEFINE_INT64_BINDING_TYPE(signed long, int64ToWire);
DEFINE_INT64_BINDING_TYPE(signed long long, int64ToWire);

} // namespace
