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

	// WrapperFlags::shared must be unset in flags!
	BindWrapper(Bound *bound, WrapperFlags flags) :
		boundUnsafe(bound), flags(flags) {}

	// WrapperFlags::shared must be set in flags!
	BindWrapper(std::shared_ptr<Bound> bound, WrapperFlags flags) :
		boundShared(bound), flags(flags) {}

	// This destructor is called automatically by the JavaScript garbage collector.

	~BindWrapper() {
		destroy();
	}

	// Pass any constructor arguments to wrapped class.
	template<typename... Args>
	static void createObj(
		const Nan::FunctionCallbackInfo<v8::Value> &nanArgs,
		Args&&... args
	) {
		(new BindWrapper(
			std::make_shared<Bound>(args...),
			WrapperFlags::shared
		))->wrapThis(nanArgs);
	}

	static void wrapPtr(const Nan::FunctionCallbackInfo<v8::Value> &nanArgs) {
		auto flags = static_cast<WrapperFlags>(nanArgs[1]->Uint32Value());
		void *ptr = v8::Handle<v8::External>::Cast(nanArgs[0])->Value();

		if(!(flags & WrapperFlags::shared)) {
			auto *ptrUnsafe = static_cast<Bound *>(ptr);

			(new BindWrapper(ptrUnsafe, flags))->wrapThis(nanArgs);
		} else {
			auto *ptrShared = static_cast<std::shared_ptr<Bound> *>(ptr);

			(new BindWrapper(*ptrShared, flags))->wrapThis(nanArgs);

			// Delete temporary shared pointer after re-referencing target object.
			delete ptrShared;
		}
	}

	void destroy() {

		// Delete the bound object if C++ side isn't holding onto it.
		if(boundShared.use_count()) boundShared.reset();

#		if !defined(DUPLICATE_POINTERS)

			// JavaScript side no longer holds any references to the object,
			// so remove our weak pointer to the wrapper.

			removeInstance();

#		endif // DUPLICATE_POINTERS

	}

	// TODO: this should throw or never get called if bound is not shared!

	inline std::shared_ptr<Bound> getShared() { return(boundShared); }

	inline WrapperFlags getFlags() const { return(flags); }

	inline Bound *getBound(WrapperFlags argFlags) {
		if(!!(flags & WrapperFlags::constant) && !(argFlags & WrapperFlags::constant)) {
			throw(std::runtime_error("Passing a const value as a non-const argument"));
		}

		return(!(flags & WrapperFlags::shared) ? boundUnsafe : boundShared.get());
	}

#if !defined(DUPLICATE_POINTERS)

	static Nan::Persistent<v8::Object> *findInstance(const Bound *ptr, WrapperFlags flags) {
		// This will insert a null pointer (to a non-existent wrapper)
		// in the map if the object pointer is not found yet.

		return(&getInstanceTbl()[HashablePair<const Bound *, WrapperFlags>(ptr, flags)]);
	}

#endif // DUPLICATE_POINTERS

private:

	void wrapThis(const Nan::FunctionCallbackInfo<v8::Value> &args) {

#		if !defined(DUPLICATE_POINTERS)

			addInstance(args.This());

#		endif // DUPLICATE_POINTERS

		this->Wrap(args.This());
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
			HashablePair<const Bound *, WrapperFlags>(
				!(flags & WrapperFlags::shared) ? boundUnsafe : boundShared.get(),
				flags
			)
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
			HashablePair<const Bound *, WrapperFlags>(
				!(flags & WrapperFlags::shared) ? boundUnsafe : boundShared.get(),
				flags
			)
		);
	}

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

	std::shared_ptr<Bound> boundShared;
	Bound *boundUnsafe;
	WrapperFlags flags;
};

} // namespace
