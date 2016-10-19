// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Holds a permanent reference to a JavaScript object, protecting it
// from garbage collection.

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

	// Call callback with data when external object is garbage collected.

	template<typename Data>
	void addDestructor(void callback(Data *), Data *data) {
		new Destructor<Data>(callback, data, handle);
	}

	v8::Local<v8::Object> getHandle() {
		return(Nan::New(handle));
	}

private:

	// Destructor info attached to an object for detecting when it gets
	// garbage collected.

	template<typename Data>
	struct Destructor {

		Destructor(
			void callback(Data *),
			Data *data,
			const Nan::Persistent<
				v8::Object,
				Nan::CopyablePersistentTraits<v8::Object>
			> &handle
		) : callback(callback), data(data), weak(Nan::New(handle)) {
			weak.SetWeak(
				this,
				&gc,
				Nan::WeakCallbackType::kParameter
			);

			weak.MarkIndependent();
		}

		// Called by V8, calls callback and deletes the destructor.

		static void gc(const Nan::WeakCallbackInfo<Destructor<Data>> &data) {
			Destructor<Data> *arg = data.GetParameter();

			arg->callback(arg->data);

			delete arg;
		}

		// Callback and its arbitrary data for notifying about destruction
		// of watched object.
		void (*callback)(Data *);
		Data *data;

		// Hold a weak reference to the object, to detect but not prevent its
		// garbage collection.
		Nan::Persistent<v8::Object> weak;

	};

	// CopyablePersistentTraits automatically resets handle in the destructor.
	Nan::Persistent<v8::Object, Nan::CopyablePersistentTraits<v8::Object>> handle;
};

template<>
struct BindingType<External> {

	typedef External Type;

	static inline bool checkType(WireType arg) { return(arg->IsObject()); }

	static inline Type fromWireType(WireType arg) { return(External(arg->ToObject())); }
	static inline WireType toWireType(Type arg) { return(arg.getHandle()); }

};

} // namespace
