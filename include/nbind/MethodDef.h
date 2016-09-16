// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Storage format for method definitions.

class BaseSignature;

class MethodDef {

public:

	MethodDef(
		const char *name,
		funcPtr ptr,
		unsigned int num,
		BaseSignature *signature,
		TypeFlags flags
	) : name(name), ptr(ptr), num(num), signature(signature), flags(flags) {}

	const char *getName() const { return(name); }
	funcPtr getPtr() const { return(ptr); }
	unsigned int getNum() const { return(num); }
	const BaseSignature *getSignature() const { return(signature); }
	TypeFlags getFlags() const { return(flags); }

private:

	const char *name;

	// Pointer to call C++ function directly, sometimes used in Emscripten.

	const funcPtr ptr;

	// Index to distinguish between functions with identical signatures.

	const unsigned int num;

	// Signature represents return and argument types.

	const BaseSignature *signature;

	const TypeFlags flags;

};

} // namespace
