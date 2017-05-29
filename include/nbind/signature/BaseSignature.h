// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Base class for signatures of all constructors, functions and methods.
// A signature represents a unique set of argument and return types,
// and the invoker functions needed to deal with such types.

class BaseSignature {

public:

	// Type of the signature.
	// Determines the actual signature class of each instance.
	// These must match JavaScript enum SignatureType in common.ts

	enum class SignatureType: unsigned int {
		none = 0,
		func,
		method,
		getter,
		setter,
		construct
	};

	BaseSignature(
		SignatureType type,
		funcPtr caller,
		const char **policyNameList,
		const TYPEID *typeList,
		unsigned int arity
	) : type(type), caller(caller), policyNameList(policyNameList), typeList(typeList), arity(arity) {}

	SignatureType getType() const { return(type); }
	funcPtr getCaller() const { return(caller); }

	// Type list is one item longer than arity,
	// because it starts with the return type (not counted in arity).

	const TYPEID *getTypeList() const { return(typeList); }
	unsigned int getArity() const { return(arity); }

	const char **getPolicies() const {
		return(policyNameList);
	}

	// A value constructor pointer is included in each signature,
	// but only used for constructors.

	funcPtr getValueConstructor() const { return(valueConstructor); }
	void setValueConstructor(funcPtr valueConstructor) {
		this->valueConstructor = valueConstructor;
	}

	template<typename MethodType>
	static inline funcPtr getDirect(MethodType func) { return(nullptr); }

private:

	const SignatureType type;
	const funcPtr caller;
	const char **policyNameList;
	const TYPEID *typeList;
	const unsigned int arity;
	funcPtr valueConstructor;

};

// Templated static class for each different function call signature exposed by the
// Node.js plugin. Used to pass arguments and return values between C++ and Node.js.
// Everything must be static because the V8 JavaScript engine wants a single
// function pointer to call, so each template variant is a singleton class.

template <class Signature, typename PolicyList, typename ReturnType, typename... Args>
class TemplatedBaseSignature : public BaseSignature {

public:

	TemplatedBaseSignature() : BaseSignature(
		Signature::typeExpr,
		reinterpret_cast<funcPtr>(Signature::call),
		PolicyLister<PolicyList>::getNameList(),
		listTypes<ReturnType, Args...>(),
		sizeof...(Args)
	) {}

	// Making the instance a direct class member fails on some platforms.
	// Maybe it registers the signature too early.

	static Signature &getInstance() {
		// Linkage for a singleton instance of each templated class.
		static Signature instance;

		// The constructor should have been called, but sometimes it wasn't!
		if(instance.getType() == SignatureType :: none) new (&instance) Signature();

		return(instance);
	}

	// Information about a single named function.
	// This wrapper around Signature::MethodType is needed because TemplatedBaseSignature itself cannot see the type directly.
	// It's passed as a CRTP argument and is not fully defined here, but inside an inner class that doesn't matter.

	struct MethodInfo {

		typedef typename Signature::MethodType MethodType;

		MethodInfo(MethodType func, TypeFlags flags) : func(func), flags(flags) {}

		const MethodType func;
		const TypeFlags flags;

	};

	static const MethodInfo &getMethod(unsigned int num) {
		return(getInstance().funcVect[num]);
	}

	template <typename MethodType>
	static unsigned int addMethod(MethodType func, TypeFlags flags) {
		auto &funcVect = getInstance().funcVect;

		funcVect.emplace_back(func, flags | TypeFlags::isMethod);

		return(funcVect.size() - 1);
	}

#if defined(BUILDING_NODE_EXTENSION)

	// Specialize static caller functions defined in Caller.h.

	typedef Caller<
		ReturnType,
		typename emscripten::internal::MapWithIndex<
			PolicyList,
			TypeList,
			ArgFromWire,
			Args...
		>::type
	> CallWrapper;

	typedef Checker<
		typename emscripten::internal::MapWithIndex<
			PolicyList,
			TypeList,
			CheckWire,
			Args...
		>::type
	> CheckWrapper;

	template <typename NanArgs>
	static bool typesAreValid(NanArgs &args) {
		return(CheckWrapper::typesAreValid(args));
	}

	template <typename NanArgs>
	static v8::Local<v8::Value> getTypeError(NanArgs &args) {
		return(CheckWrapper::getTypeError(args, getInstance().getTypeList()));
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
	static Bound *getTargetSafely(
		NanArgs &nanArgs,
		Bound *target,
		TypeFlags flags
	) {
		return(BindWrapper<Bound>::getBound(nanArgs.This(), flags));
	}

	// Overload second argument, effectively a partial specialization of the function template above.
	template <typename NanArgs>
	static void *getTargetSafely(
		NanArgs &nanArgs,
		void *target,
		TypeFlags flags
	) {
		return(nullptr);
	}

	template <typename Bound, typename V8Args, typename NanArgs>
	static void callInnerSafely(V8Args &args, NanArgs &nanArgs, unsigned int methodNum) {
		Bound *target = nullptr;

		if(!arityIsValid(nanArgs)) {
			// TODO: When function is overloaded, this test could be skipped...

			std::string msg = "Wrong number of arguments, expected " + std::to_string(sizeof...(Args));
			Nan::ThrowError(msg.c_str());
			return;
		}

		if(!Signature::typesAreValid(args)) {
			Nan::ThrowError(Signature::getTypeError(args));
			return;
		}

		const MethodInfo &method = getMethod(methodNum);

		try {
			target = getTargetSafely(nanArgs, target, method.flags);

			Status::clearError();
			Signature::callInner(method, args, nanArgs, target);
			if(Status::getError() != nullptr) Nan::ThrowError(Status::getError());
		} catch(const cbException &ex) {
			// A JavaScript exception is already heading up the stack.
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
