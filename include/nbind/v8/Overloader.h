// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

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

// It would be possible to add the bound C++ class as a template argument,
// allowing unlimited classes and a per-class instead of a global limit of overloaded methods,
// but that would compile to several completely identical duplicate copies of code.

class Overloader {

public:

	typedef Nan::FunctionCallback jsMethod;
	typedef void (*createValueHandler)(ArgStorage &storage, const Nan::FunctionCallbackInfo<v8::Value> &args);

	struct OverloadDef {
		std::vector<funcPtr> methodVect;

		// Constructor called by JavaScript's "new" operator.
		Nan::Callback *constructorJS = nullptr;
	};

	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		static std::vector<OverloadDef> &overloadVect = overloadVectStore();
		// The static cast silences a compiler warning in Visual Studio.
		OverloadDef &def = overloadVect[static_cast<unsigned int>(args.Data()->IntegerValue()) >> overloadShift];

		std::vector<funcPtr> &methodVect = def.methodVect;
		unsigned int argc = args.Length();
		signed int maxArity = methodVect.size() - 1;

		// Check if method was called with more than the maximum number
		// of arguments it can accept.
		if(signed(argc) <= maxArity) {
			jsMethod specializedCall = reinterpret_cast<jsMethod>(methodVect[argc]);

			if(specializedCall != nullptr) {
				specializedCall(args);
				return;
			}
		}

		Nan::ThrowError("Wrong number of arguments");
	}

	static void callConstructor(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		Status::clearError();

		// Call C++ constructor and bind the resulting object
		// to the new JavaScript object being created.

		try {
			call(args);

			const char *message = Status::getError();

			if(message) {
				Nan::ThrowError(message);
				return;
			}

			args.GetReturnValue().Set(args.This());
		} catch(const std::exception &ex) {
			const char *message = Status::getError();

			if(message == nullptr) message = ex.what();

			Nan::ThrowError(message);
		}
	}

	static void callNew(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		// The static cast silences a compiler warning in Visual Studio.
		OverloadDef &def = getDef(static_cast<unsigned int>(args.Data()->IntegerValue()) >> overloadShift);

		unsigned int argc = args.Length();
		std::vector<v8::Local<v8::Value>> argv(argc);

		// Copy arguments to a vector because the arguments object type
		// cannot be passed to another function call as-is.

		for(unsigned int argNum = 0; argNum < argc; argNum++) {
			argv[argNum] = args[argNum];
		}

		v8::Local<v8::Function> constructor = def.constructorJS->GetFunction();

		// Call the JavaScript constructor with the new operator.
		args.GetReturnValue().Set(constructor->NewInstance(argc, &argv[0]));
	}

	static void create(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		if(args.IsConstructCall()) {
			// Constructor was called like new Bound(...)
			callConstructor(args);
		} else {
			// Constructor was called like Bound(...), add the "new" operator.
			callNew(args);
		}
	}

	static void createValue(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		ArgStorage &storage = *static_cast<ArgStorage *>(v8::Handle<v8::External>::Cast(args.Data())->Value());
		static std::vector<OverloadDef> &overloadVect = overloadVectStore();
		OverloadDef &def = overloadVect[storage.getOverloadNum()];

		std::vector<funcPtr> &methodVect = def.methodVect;
		unsigned int argc = args.Length();
		signed int maxArity = methodVect.size() - 1;

		// Check if method was called with more than the maximum number
		// of arguments it can accept.
		if(signed(argc) <= maxArity) {
			auto specializedCall = reinterpret_cast<createValueHandler>(methodVect[argc]);

			if(specializedCall != nullptr) {
				specializedCall(storage, args);
				args.GetReturnValue().Set(Nan::Undefined());
				return;
			}
		}

		// Can't use throw here because there's V8 code
		// (lacking C++ exception support) on the stack
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

	static void addMethod(unsigned int num, unsigned int arity, funcPtr method) {
		OverloadDef &def = getDef(num);

		// Get methods in overloaded group.
		std::vector<funcPtr> &methodVect = def.methodVect;

		signed int oldArity = methodVect.size() - 1;

		// Grow list of methods if new arity doesn't fit.
		if(signed(arity) > oldArity) methodVect.resize(arity + 1);

		methodVect[arity] = method;
	}

	static void setConstructorJS(unsigned int num, v8::Local<v8::Function> func) {
		OverloadDef &def = getDef(num);

		if(def.constructorJS == nullptr) {
			def.constructorJS = new Nan::Callback(func);
		} else {
			def.constructorJS->SetFunction(func);
		}
	}

	static OverloadDef &getDef(unsigned int num) {
		return(overloadVectStore()[num]);
	}

	// Linkage for a table of overloaded methods
	// (overloads must have different arities).

	static std::vector<OverloadDef> &overloadVectStore() {
		static std::vector<OverloadDef> overloadVect;
		return(overloadVect);
	}

};

} // namespace
