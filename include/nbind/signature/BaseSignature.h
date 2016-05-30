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

	enum class Type: unsigned int { function, method, getter, setter, constructor };

	BaseSignature(Type type, funcPtr caller, const TYPEID *typeList, unsigned int arity) :
		type(type), caller(caller), typeList(typeList), arity(arity) {}

	Type getType() const { return(type); }
	funcPtr getCaller() const { return(caller); }

	// Type list is one item longer than arity,
	// because it starts with the return type (not counted in arity).

	const TYPEID *getTypeList() const { return(typeList); }
	unsigned int getArity() const { return(arity); }

	// A value constructor pointer is included in each signature,
	// but only used for constructors.

	funcPtr getValueConstructor() const { return(valueConstructor); }
	void setValueConstructor(funcPtr valueConstructor) {
		this->valueConstructor = valueConstructor;
	}

	template<typename MethodType>
	static inline funcPtr getDirect(MethodType func) { return(nullptr); }

private:

	const Type type;
	const funcPtr caller;
	const TYPEID *typeList;
	const unsigned int arity;
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

	// Making the instance a direct class member fails on some platforms.
	// Maybe it registers the signature too early.

	static Signature &getInstance() {
		// Linkage for a singleton instance of each templated class.
		static Signature instance;
		return(instance);
	}

	// Information about a single named function.
	// This wrapper around Signature::MethodType is needed because TemplatedBaseSignature itself cannot see the type directly.
	// It's passed as a CRTP argument and is not fully defined here, but inside an inner class that doesn't matter.

	struct MethodInfo {

		typedef typename Signature::MethodType MethodType;

		MethodInfo(MethodType func) : func(func) {}

		const MethodType func;

	};

	static const MethodInfo &getMethod(unsigned int num) {
		return(getInstance().funcVect[num]);
	}

	template <typename MethodType>
	static unsigned int addMethod(MethodType func) {
		auto &funcVect = getInstance().funcVect;

#		if defined(BUILDING_NODE_EXTENSION)

			if(funcVect.size() >= signatureMemberMask) {
				// TODO:
				// ABORT ABORT ABORT too many functions with the same signature!
			}

#		endif // BUILDING_NODE_EXTENSION

		funcVect.emplace_back(func);

		return(funcVect.size() - 1);
	}

#if defined(BUILDING_NODE_EXTENSION)

	// Specialize static caller functions defined in Caller.h.

	typedef Caller<
		ReturnType,
		typename emscripten::internal::MapWithIndex<
			TypeList,
			ArgFromWire,
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

	static bool arityIsValid(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		static constexpr decltype(args.Length()) arity = sizeof...(Args);
		return(args.Length() == arity);
	}

	template <typename Value>
	static bool arityIsValid(const Nan::PropertyCallbackInfo<Value> &args) {
		return(true);
	}

	template <typename NanArgs, typename Bound>
	static bool getTargetSafely(NanArgs &nanArgs, Bound **targetOut) {
		v8::Local<v8::Object> targetWrapped = nanArgs.This();
		Bound *target = node::ObjectWrap::Unwrap<BindWrapper<Bound>>(targetWrapped)->getBound();

		if(target == nullptr) return(false);

		*targetOut = target;
		return(true);
	}

	// Overload second argument, effectively a partial specialization of the function template above.
	template <typename NanArgs>
	static bool getTargetSafely(NanArgs &nanArgs, void **targetOut) {
		return(true);
	}

	template <typename Bound, typename V8Args, typename NanArgs>
	static void callInnerSafely(V8Args &args, NanArgs &nanArgs) {
		Bound *target = nullptr;

		if(!arityIsValid(nanArgs)) {
			// TODO: When function is overloaded, this test could be skipped...
			Nan::ThrowError("Wrong number of arguments");
			return;
		}

		if(!Signature::typesAreValid(args)) {
			Nan::ThrowTypeError("Type mismatch");
			return;
		}

		if(!getTargetSafely(nanArgs, &target)) {
			Nan::ThrowError("Attempt to access deleted object");
			return;
		}

		Status::clearError();

		try {
			Signature::callInner(args, nanArgs, target);
			if(Status::getError() != nullptr) Nan::ThrowError(Status::getError());
		} catch(const std::exception &ex) {
			const char *message = Status::getError();

			if(message == nullptr) message = ex.what();

			Nan::ThrowError(message);
		}
	}

#endif // BUILDING_NODE_EXTENSION

	// The funcVect vector cannot be moved to BaseSignature because it can contain pointers to
	// functions or class methods, and there isn't a single pointer type able to hold both.

	std::vector<struct MethodInfo> funcVect;

};

} // namespace
