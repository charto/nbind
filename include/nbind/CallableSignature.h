// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Templated static class for each different function call signature exposed by the
// Node.js plugin. Used to pass arguments and return values between C++ and Node.js.
// Everything must be static because the V8 JavaScript engine wants a single
// function pointer to call, so each template variant is a singleton class.

#ifdef EMSCRIPTEN
template<typename ArgType> constexpr char emMangle();

template<> constexpr char emMangle<int>() {return('i');}
template<> constexpr char emMangle<void>() {return('v');}
template<> constexpr char emMangle<float>() {return('f');}
template<> constexpr char emMangle<double>() {return('d');}

template<typename... Args>
const char* getEmSignature() {
	static constexpr char signature[] = { emMangle<Args>()..., 0 };

	return(signature);
}

template<typename ArgType> struct EmMangleMap { typedef int type; };
template<> struct EmMangleMap<void> { typedef void type; };
template<> struct EmMangleMap<float> { typedef float type; };
template<> struct EmMangleMap<double> { typedef double type; };
#endif // EMSCRIPTEN

template <class Signature, typename ReturnType, typename... Args>
class CallableSignature {

public:

	// Information about a single named function.

	struct MethodInfo {

		typedef typename Signature::MethodType MethodType;

		MethodInfo(MethodType func) : func(func) {}

		MethodType func;

	};

	// Information about a class of functions with matching return and argument types.

	struct SignatureInfo {
		std::vector<struct MethodInfo> funcVect;
#ifdef EMSCRIPTEN
		const char *emSignature = getEmSignature<typename EmMangleMap<ReturnType>::type, typename EmMangleMap<Args>::type...>();
#endif
	};

	static const MethodInfo &getMethod(unsigned int num) {
		return(signatureStore().funcVect[num]);
	}

	template <typename MethodType>
	static unsigned int addMethod(MethodType func) {
		auto &funcVect = signatureStore().funcVect;

		funcVect.emplace_back(func);

		return(funcVect.size() - 1);
	}

#ifdef BUILDING_NODE_EXTENSION
	typedef Caller<
		ReturnType,
		typename emscripten::internal::MapWithIndex<
			TypeList,
			FromWire,
			Args...
		>::type
	> CallWrapper;

	template <typename NanArgs>
	static bool typesAreValid(NanArgs &args) {
		typedef Checker<
			typename emscripten::internal::MapWithIndex<
				TypeList,
				CheckWire,
				Args...
			>::type
		> checker;

		return(checker::typesAreValid(args));
	}
#elif EMSCRIPTEN
#endif // BUILDING_NODE_EXTENSION || EMSCRIPTEN

protected:

	static SignatureInfo &signatureStore() {
		static SignatureInfo signatureInfo;
		return(signatureInfo);
	}

};

} // namespace
