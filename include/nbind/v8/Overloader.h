// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
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

		jsMethod wrapPtr = nullptr;
	};

	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		// Fetch overloads of the requested function.
		OverloadDef &def = getDef(SignatureParam::get(args)->overloadNum);

		unsigned int argc = args.Length();

		// If the only argument is a pointer, assume we just want to wrap
		// an already instantiated object.

		if(argc == 2 && args[0]->IsExternal()) {
			def.wrapPtr(args);
			return;
		}

		std::vector<funcPtr> &methodVect = def.methodVect;
		signed int maxArity = methodVect.size() - 1;

		// Check that the method wasn't called with more than
		// its maximum number of arguments.

		if(signed(argc) <= maxArity) {
			// Get caller for requested arity.
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
		} catch(const cbException &ex) {
			// A JavaScript exception is already heading up the stack.
		} catch(const std::exception &ex) {
			const char *message = Status::getError();

			if(message == nullptr) message = ex.what();

			Nan::ThrowError(message);
		}
	}

	static void callNew(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		OverloadDef &def = getDef(SignatureParam::get(args)->overloadNum);

		unsigned int argc = args.Length();
		std::vector<v8::Local<v8::Value>> argv(argc);

		// Copy arguments to a vector because the arguments object type
		// cannot be passed to another function call as-is.

		for(unsigned int argNum = 0; argNum < argc; argNum++) {
			argv[argNum] = args[argNum];
		}

		v8::Local<v8::Function> constructor = def.constructorJS->GetFunction();

		// Call the JavaScript constructor with the new operator.
		auto result = Nan::NewInstance(constructor, argc, (argc)?(&argv[0]):(nullptr));

		if(result.IsEmpty()) args.GetReturnValue().Set(Nan::Undefined());
		else args.GetReturnValue().Set(result.ToLocalChecked());
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
		ArgStorage &storage = *static_cast<ArgStorage *>(Nan::GetInternalFieldPointer(args.Holder(), 0));
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

	static void setPtrWrapper(unsigned int num, jsMethod wrapPtr) {
		getDef(num).wrapPtr = wrapPtr;
	}

	static inline OverloadDef &getDef(unsigned int num) {
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
