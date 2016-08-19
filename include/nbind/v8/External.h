// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class External {

public:

	External() {}

	explicit External(v8::Local<v8::Object> obj) : handle(obj) {}

	External(const External &other) : handle(Nan::New(other.handle)) {}

	External(External &&other) : handle(Nan::New(other.handle)) {
		other.handle.Reset();
	}

	External &operator=(const External &other) {
		handle.Reset(Nan::New(other.handle));

		return(*this);
	}

	External &operator=(v8::Local<v8::Object> obj) {
		handle.Reset(obj);

		return(*this);
	}

	External &operator=(External &&other) {
		handle.Reset(Nan::New(other.handle));
		other.handle.Reset();

		return(*this);
	}

protected:

	// CopyablePersistentTraits automatically resets handle in the destructor.
	Nan::Persistent<v8::Object, Nan::CopyablePersistentTraits<v8::Object>> handle;
};

} // namespace
