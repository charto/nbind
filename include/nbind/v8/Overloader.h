// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// Unite:
// ConstructorOverload.h:     static void valueConstructorCaller(const Nan::FunctionCallbackInfo<v8::Value> &args) {
// ConstructorSignature.h: void ConstructorOverload<Bound>::call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
// FunctionSignature.h:                         static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
// MethodSignature.h:                           static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
// Note: We can't overload getters and setters in the same way! They always take the same number of arguments.



/*
This should contain a vector of overloaded methods.
Each method is defined by a vector of method signatures.
The v8 engine is then given a pointer to the call function here and a reference number.
The most significant bits (>> overloadShift)
of the reference number contain an index to the vector of overloaded methods.
When a call is made to the overloader, the correct signature is found based on the number of arguments to the call.
The signature's caller is then called, and it extracts the correct method among those with an identical signature,
based on the least significant bits (& signatureMemberMask) of the reference number.
*/



#pragma once

namespace nbind {

static constexpr unsigned int overloadShift = 16;
static constexpr unsigned int signatureMemberMask = 0xffff;

// It would be possible to add the bound C++ class as a template argument,
// allowing unlimited classes and a per-class instead of a global limit of overloaded methods,
// but that would compile to several completely identical duplicate copies of code.

class Overloader {

public:

	typedef Nan::FunctionCallback jsMethod;

	struct OverloadDef {
		std::vector<jsMethod> methodVect;
	};

	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		static std::vector<OverloadDef> &overloadVect = overloadVectStore();
		OverloadDef &def = overloadVect[args.Data()->IntegerValue() >> overloadShift];

		std::vector<jsMethod> &methodVect = def.methodVect;
		unsigned int argc = args.Length();
		signed int maxArity = methodVect.size() - 1;

		// Check if method was called with more than the maximum number
		// of arguments it can accept.
		if(signed(argc) <= maxArity) {
			auto specializedCall = methodVect[argc];

			if(specializedCall != nullptr) {
				specializedCall(args);
				return;
			}
		}

		// Can't use throw here because we might be calling a value object constructor.
		// In that case there's V8 code (lacking C++ exception support) on the stack
		// above the catch statement.

		NBIND_ERR("Wrong number of arguments in value binding");
		args.GetReturnValue().Set(Nan::Undefined());
	}

	static unsigned int addGroup() {
		std::vector<OverloadDef> &overloadVect = overloadVectStore();
		unsigned int num = overloadVect.size();

		// Add a new group of overloaded methods.
		overloadVect.resize(num + 1);

		return(num);
	}

	static void addMethod(unsigned int num, unsigned int arity, jsMethod method) {
		std::vector<OverloadDef> &overloadVect = overloadVectStore();
		OverloadDef &def = overloadVect[num];

		// Get methods in overloaded group.
		std::vector<jsMethod> &methodVect = def.methodVect;

		signed int oldArity = methodVect.size() - 1;

		// Grow list of methods if new arity doesn't fit.
		if(signed(arity) > oldArity) methodVect.resize(arity + 1);

		methodVect[arity] = method;
	}

	// Linkage for a table of overloaded methods
	// (overloads must have different arities).

	static std::vector<OverloadDef> &overloadVectStore() {
		static std::vector<OverloadDef> overloadVect;
		return(overloadVect);
	}

};

} // namespace
