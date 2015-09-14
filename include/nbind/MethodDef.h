// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Storage format for method definitions.

class BaseSignature;

class MethodDef {

public:

	MethodDef(const char *name, funcPtr ptr, unsigned int num, BaseSignature *signature) :
		name(name), ptr(ptr), num(num), signature(signature) {}

	const char *getName() {return(name);}
	funcPtr getPtr() { return(ptr); }
	unsigned int getNum() {return(num);}
	BaseSignature *getSignature() {return(signature);}

private:

	const char *name;

	// Pointer to call C++ function directly, sometimes used in Emscripten.

	funcPtr ptr;

	// Index to distinguish between functions with identical signatures.

	unsigned int num;

	// Signature represents return and argument types.

	BaseSignature *signature;

};

} // namespace
