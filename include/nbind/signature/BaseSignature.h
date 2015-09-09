// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class BaseSignature {

public:

	enum class Type {function, method, getter, setter, constructor};

	BaseSignature(Type type, funcPtr caller, const TYPEID *typeList, unsigned int typeCount) :
		type(type), caller(caller), typeList(typeList), typeCount(typeCount) {}

	Type getType() { return(type); }
	funcPtr getCaller() { return(caller); }
	const TYPEID *getTypeList() { return(typeList); }
	unsigned int getTypeCount() { return(typeCount); }

private:

	Type type;
	funcPtr caller;
	const TYPEID *typeList;
	unsigned int typeCount;

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
		reinterpret_cast<funcPtr>(Signature::call),
		listTypes<ReturnType, Args...>(),
		sizeof...(Args) + 1
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

#ifdef BUILDING_NODE_EXTENSION
		if(funcVect.size() >= signatureMemberMask) {
			// TODO:
			// ABORT ABORT ABORT too many functions with the same signature!
		}
#endif // BUILDING_NODE_EXTENSION

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
#endif // BUILDING_NODE_EXTENSION || EMSCRIPTEN

	// The funcVect vector cannot be moved to BaseSignature because it can contain pointers to
	// functions or class methods, and there isn't a single pointer type able to hold both.

	std::vector<struct MethodInfo> funcVect;

};

} // namespace
