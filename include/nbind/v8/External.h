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

	template<typename Data>
	void addDestructor(void callback(Data *), Data *data) {
		new Destructor<Data>(callback, data, handle);
	}

private:

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

		static void gc(const Nan::WeakCallbackInfo<Destructor<Data>> &data) {
			Destructor<Data> *arg = data.GetParameter();

			arg->callback(arg->data);

			delete arg;
		}

		void (*callback)(Data *);
		Data *data;
		Nan::Persistent<v8::Object> weak;

	};

	// CopyablePersistentTraits automatically resets handle in the destructor.
	Nan::Persistent<v8::Object, Nan::CopyablePersistentTraits<v8::Object>> handle;
};

} // namespace
