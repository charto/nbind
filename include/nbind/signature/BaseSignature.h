// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class BaseSignature {

public:

	enum class Type {function, method, getter, setter};

	BaseSignature(Type type, funcPtr caller) : type(type), caller(caller) {}

	Type getType() { return(type); }
	funcPtr getCaller() { return(caller); }

private:

	Type type;
	funcPtr caller;

};

// Templated static class for each different function call signature exposed by the
// Node.js plugin. Used to pass arguments and return values between C++ and Node.js.
// Everything must be static because the V8 JavaScript engine wants a single
// function pointer to call, so each template variant is a singleton class.

template <class Signature, typename ReturnType, typename... Args>
class TemplatedBaseSignature : public BaseSignature {

public:

	TemplatedBaseSignature() : BaseSignature(
		Signature::typeExpr,
		reinterpret_cast<funcPtr>(Signature::call)
	) {}

	static BaseSignature &getInstance() {
		static Signature instance;
		return(instance);
	}

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
		const char *emSignature = buildEmSignature<typename EmMangleMap<ReturnType>::type, typename EmMangleMap<Args>::type...>();
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
	static const char *getEmSignature() {
		return(signatureStore().emSignature);
	}
#endif // BUILDING_NODE_EXTENSION || EMSCRIPTEN

protected:

	static SignatureInfo &signatureStore() {
		static SignatureInfo signatureInfo;
		return(signatureInfo);
	}

};

} // namespace
