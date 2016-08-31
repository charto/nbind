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

template <typename A, typename B>
class HashablePair : public std::pair<A, B> {

public:

	HashablePair(A a, B b) : std::pair<A, B>(a, b) {}

};

}

namespace std {

// Add hash functions for HashablePair and WrapperFlags so they can be used
// as keys in std::unordered_map.

template<typename A, typename B> struct hash<nbind::HashablePair<A, B>> {
	inline size_t operator()(const nbind::HashablePair<A, B> &obj) const {
		size_t result = hash<A>()(obj.first) + 0x9e3779b9;

		result = (result << 6) + (result >> 2);
		result += hash<B>()(obj.second) + 0x9e3779b9;

		return(result);
	}
};

template<>
struct hash<nbind::WrapperFlags> {
	inline size_t operator()(nbind::WrapperFlags flags) const {
		return(hash<uint32_t>()(static_cast<uint32_t>(flags)));
	}
};

}

namespace nbind {

// BindWrapper encapsulates a C++ object created in Node.js.

template <class Bound>
class BindWrapper : public node::ObjectWrap {

public:

	BindWrapper(Bound *bound, WrapperFlags flags) : bound(bound), flags(flags) {}

	// This destructor is called automatically by the JavaScript garbage collector.

	~BindWrapper() {
		destroy();
	}

	// Pass any constructor arguments to wrapped class.
	template<typename... Args>
	static BindWrapper *createObj(Args&&... args) {
		return(new BindWrapper(new Bound(args...), WrapperFlags::none));
	}

	static void wrapPtr(const Nan::FunctionCallbackInfo<v8::Value> &args) {
		Bound *ptr = static_cast<Bound *>(v8::Handle<v8::External>::Cast(args[0])->Value());

		(new BindWrapper(
			ptr,
			static_cast<WrapperFlags>(args[1]->Uint32Value())
		))->wrapThis(args);
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

#		if !defined(DUPLICATE_POINTERS)

			addInstance(args.This());

#		endif // DUPLICATE_POINTERS

		this->Wrap(args.This());
	}

	inline std::shared_ptr<Bound> getShared() { return(bound); }

	inline WrapperFlags getFlags() const { return(flags); }

	inline Bound *getBound(WrapperFlags argFlags) {
		if(!!(flags & WrapperFlags::constant) && !(argFlags & WrapperFlags::constant)) {
			throw(std::runtime_error("Passing a const value as a non-const argument"));
		}

		return(bound.get());
	}

#if !defined(DUPLICATE_POINTERS)

	// If the GC wants to free the wrapper object, get rid of our reference to it.

	static void weakCallback(const Nan::WeakCallbackInfo<Nan::Persistent<v8::Object>> &data) {
		// This causes a crash. Maybe it's a reference counter that already got decremented?
		// data.GetParameter()->Reset();
	}

	// Add a mapping from a pointer to a wrapper object,
	// to re-use the same wrapper for duplicates of the same pointer.

	void addInstance(v8::Local<v8::Object> obj) {
		Nan::Persistent<v8::Object> *ref = &getInstanceTbl()[
			HashablePair<const Bound *, WrapperFlags>(bound.get(), flags)
		];

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

		getInstanceTbl().erase(
			HashablePair<const Bound *, WrapperFlags>(bound.get(), flags)
		);
	}

	static Nan::Persistent<v8::Object> *findInstance(const Bound *ptr, WrapperFlags flags) {
		// This will insert a null pointer (to a non-existent wrapper)
		// in the map if the object pointer is not found yet.

		return(&getInstanceTbl()[HashablePair<const Bound *, WrapperFlags>(ptr, flags)]);
	}

#endif // DUPLICATE_POINTERS

private:

#if !defined(DUPLICATE_POINTERS)

	// This is effectively a map from C++ instance pointers
	// to weak references to JavaScript objects wrapping them.

	static std::unordered_map<
		HashablePair<const Bound *, WrapperFlags>,
		Nan::Persistent<v8::Object>
	> &getInstanceTbl() {
		static std::unordered_map<
			HashablePair<const Bound *, WrapperFlags>,
			Nan::Persistent<v8::Object>
		> instanceTbl;

		return(instanceTbl);
	}

#endif // DUPLICATE_POINTERS

	std::shared_ptr<Bound> bound;
	WrapperFlags flags;
};

} // namespace
