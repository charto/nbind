// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class BaseSignature {

public:

	enum class Type {function, method, getter, setter};

	BaseSignature(Type type, funcPtr caller, const TYPEID *typeList) :
		type(type), caller(caller), typeList(typeList) {}

	Type getType() { return(type); }
	funcPtr getCaller() { return(caller); }
	const TYPEID *getTypeList() { return(typeList); }

private:

	Type type;
	funcPtr caller;
	const TYPEID *typeList;

};

// Templated static class for each different function call signature exposed by the
// Node.js plugin. Used to pass arguments and return values between C++ and Node.js.
// Everything must be static because the V8 JavaScript engine wants a single
// function pointer to call, so each template variant is a singleton class.

template <class Signature, typename ReturnType, typename... Args>
class TemplatedBaseSignature : public BaseSignature {

public:

	TemplatedBaseSignature(const TYPEID *typeList) : BaseSignature(
		Signature::typeExpr,
		reinterpret_cast<funcPtr>(Signature::call),
		typeList
	) {}

	static Signature &getInstance() {
		static Signature instance;
		return(instance);
	}

	// Information about a single named function.
	// This wrapper around Signature::MethodType is needed because TemplatedBaseSignature itself cannot see the type directly.
	// It's passed as a CRTP argument and is not fully defined here, but inside an inner class that doesn't matter.

	struct MethodInfo {

		typedef typename Signature::MethodType MethodType;

		MethodInfo(MethodType func) : func(func) {}

		MethodType func;

	};

	static const MethodInfo &getMethod(unsigned int num) {
		return(getInstance().funcVect[num]);
	}

	template <typename MethodType>
	static unsigned int addMethod(MethodType func) {
		auto &funcVect = getInstance().funcVect;

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
		return(getInstance().emSignature);
	}
#endif // BUILDING_NODE_EXTENSION || EMSCRIPTEN

	// The funcVect vector cannot be moved to BaseSignature because it can contain pointers to
	// functions or class methods, and there isn't a single pointer type able to hold both.

	std::vector<struct MethodInfo> funcVect;
#ifdef EMSCRIPTEN
	const char *emSignature = buildEmSignature<typename EmMangleMap<ReturnType>::type, typename EmMangleMap<Args>::type...>();
#endif

};

} // namespace
