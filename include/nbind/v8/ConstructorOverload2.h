// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

//template <class Bound>
//class CreateSignature {

//public:

template <class Bound>
	void ConstructorOverload<Bound>::call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
//	static void call(const Nan::FunctionCallbackInfo<v8::Value> &args) {
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
				constructor(args)->wrap(args);

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
//}

} // namespace
