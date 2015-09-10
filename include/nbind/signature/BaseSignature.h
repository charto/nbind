// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Constants for packing several method signatures into a single overloaded name,
// and several methods with identical argument and return types into a single signature.

static constexpr unsigned int overloadShift = 16;
static constexpr unsigned int signatureMemberMask = 0xffff;

// Base class for signatures of all constructors, functions and methods.
// A signature represents a unique set of argument and return types,
// and the invoker functions needed to deal with such types.

class BaseSignature {

public:

	// Type of the signature.
	// Determines the actual signature class of each instance.

	enum class Type {function, method, getter, setter, constructor};

	BaseSignature(Type type, funcPtr caller, const TYPEID *typeList, unsigned int arity) :
		type(type), caller(caller), typeList(typeList), arity(arity) {}

	Type getType() { return(type); }
	funcPtr getCaller() { return(caller); }

	// Type list is one item longer than arity,
	// because it starts with the return type (not counted in arity).

	const TYPEID *getTypeList() { return(typeList); }
	unsigned int getArity() { return(arity); }

	// A value constructor pointer is included in each signature,
	// but only used for constructors.

	funcPtr getValueConstructor() { return(valueConstructor); }
	void setValueConstructor(funcPtr valueConstructor) {
		this->valueConstructor = valueConstructor;
	}

private:

	Type type;
	funcPtr caller;
	const TYPEID *typeList;
	unsigned int arity;
	funcPtr valueConstructor;

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
		sizeof...(Args)
	) {}

	// Linkage for a singleton instance of each templated class.

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
	// Specialize static caller functions defined in Caller.h.

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
#endif // BUILDING_NODE_EXTENSION

	// The funcVect vector cannot be moved to BaseSignature because it can contain pointers to
	// functions or class methods, and there isn't a single pointer type able to hold both.

	std::vector<struct MethodInfo> funcVect;

};

} // namespace
