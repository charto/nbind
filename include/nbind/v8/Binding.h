// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#ifdef BUILDING_NODE_EXTENSION

#define NBIND 1

namespace nbind {

// Storage for class bindings, populated by BindDefiners created in
// constructors of static BindInvoker objects.

class Bindings {

public:

	static void registerClass(BindClassBase *bindClass);
	static void initModule(v8::Handle<v8::Object> exports);
	static void setValueConstructorByName(const char *name, cbFunction &func);

private:

	// Linkage for a list of all C++ class wrappers.

	static std::forward_list<BindClassBase *> &getClassList() {
		static std::forward_list<BindClassBase *> classList;
		return(classList);
	}

};

extern v8::Persistent<v8::Object> constructorStore;

// The create function would better fit in BindClass but it needs to call
// node::ObjectWrap::Wrap which is protected and only inherited by BindWrapper.
//template <class Bound>
//NAN_METHOD(BindClass<Bound>::create) {
//	return(BindWrapper<Bound>::create(args));
//}

template <class Bound>
void BindWrapper<Bound>::create(const Nan::FunctionCallbackInfo<v8::Value> &args) {
//NAN_METHOD(BindWrapper<Bound>::create) {
	if(args.IsConstructCall()) {
		// Constructor was called like new Bound(...)

		// Look up possibly overloaded C++ constructor according to its arity
		// in the constructor call.
		auto *constructor = BindClass<Bound>::getWrapperConstructor(args.Length());

		if(constructor == nullptr) {
			Nan::ThrowError("Wrong number of arguments");
			return;
		}

		Status::clearError();

		// Call C++ constructor and bind the resulting object
		// to the new JavaScript object being created.
		try {
			constructor(args)->Wrap(args.This());

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
	} else {
		// Constructor was called like Bound(...), add the "new" operator.

		unsigned int argc = args.Length();
		std::vector<v8::Handle<v8::Value>> argv(argc);

		// Copy arguments to a vector because the arguments object type
		// cannot be passed to another function call as-is.
		for(unsigned int argNum = 0; argNum < argc; argNum++) {
			argv[argNum] = args[argNum];
		}

		v8::Handle<v8::Function> constructor = BindClass<Bound>::getInstance()->getConstructorHandle();

		// Call the JavaScript constructor with the new operator.
		args.GetReturnValue().Set(constructor->NewInstance(argc, &argv[0]));
	}
}

} // namespace

#endif // BUILDING_NODE_EXTENSION
