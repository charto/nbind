// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <memory>

#if !defined(DUPLICATE_POINTERS)

	#include <unordered_map>

#endif // DUPLICATE_POINTERS

#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <nan.h>

namespace nbind {

// BindWrapper encapsulates a C++ object created in Node.js.

template <class Bound>
class BindWrapper : public node::ObjectWrap {

public:

	BindWrapper(Bound *bound) : bound(bound) {}

	// This destructor is called automatically by the JavaScript garbage collector.

	~BindWrapper() {
		destroy();
	}

	// Pass any constructor arguments to wrapped class.
	template<typename... Args>
	static BindWrapper *createObj(Args&&... args) {
		return(new BindWrapper(new Bound(args...)));
	}

	static BindWrapper *createPtr(Bound *ptr) {
		return(new BindWrapper(ptr));
	}

	void destroy() {
		if(bound) {
			// Note: for thread safety, this block should be a critical section.

#			if !defined(DUPLICATE_POINTERS)

				removeInstance();

#			endif // DUPLICATE_POINTERS

			// This deletes the bound object if necessary.
			bound.reset();
		}
	}

	void wrapThis(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		addInstance(args.This());

		this->Wrap(args.This());
	}

	Bound *getBound() { return(bound.get()); }

#if !defined(DUPLICATE_POINTERS)

	// If the GC wants to free the wrapper object, get rid of our reference to it.

	static void weakCallback(const Nan::WeakCallbackInfo<Nan::Persistent<v8::Object>> &data) {
		// This causes a crash. Maybe it's a reference counter that already got decremented?
		// data.GetParameter()->Reset();
	}

	// Add a mapping from a pointer to a wrapper object,
	// to re-use the same wrapper for duplicates of the same pointer.

	void addInstance(v8::Local<v8::Object> obj) {
		Nan::Persistent<v8::Object> *ref = &getInstanceTbl()[bound.get()];

		ref->Reset(obj);

		// Mark the reference to the wrapper weak, so it can be
		// garbage collected when no copies remain.
		// If the same pointer needs wrapping later, we can just
		// create a new wrapper again.

		ref->SetWeak(ref, weakCallback, Nan::WeakCallbackType::kParameter);
		ref->MarkIndependent();
	}

	void removeInstance() {
		// This causes a crash. Maybe it's a reference counter that already got decremented?
		// auto iter = getInstanceTbl().find(bound.get());
		// (*iter).second.Reset();

		getInstanceTbl().erase(bound.get());
	}

	static Nan::Persistent<v8::Object> *findInstance(Bound *ptr) {
		// This will insert a null pointer (to a non-existent wrapper)
		// in the map if the object pointer is not found yet.

		return(&getInstanceTbl()[ptr]);
	}

#endif // DUPLICATE_POINTERS

private:

#if !defined(DUPLICATE_POINTERS)

	// This is effectively a map from C++ instance pointers
	// to weak references to JavaScript objects wrapping them.

	static std::unordered_map<Bound *, Nan::Persistent<v8::Object>> &getInstanceTbl() {
		static std::unordered_map<Bound *, Nan::Persistent<v8::Object>> instanceTbl;
		return(instanceTbl);
	}

#endif // DUPLICATE_POINTERS

	std::shared_ptr<Bound> bound;
};

} // namespace
