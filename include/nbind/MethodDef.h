// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Storage format for method definitions.

class BaseSignature;

class MethodDef {

public:

	MethodDef(const char *name, unsigned int num, BaseSignature *signature) :
		name(name), num(num), signature(signature) {}

	const char *getName() {return(name);}
	unsigned int getNum() {return(num);}
	BaseSignature *getSignature() {return(signature);}

#ifdef EMSCRIPTEN
	void emInit(const char *emSignature) {
		this->emSignature = emSignature;
	}

	const char *getEmSignature() {
		return(emSignature);
	}
#endif

private:

	const char *name;
	// Index to distinguish between functions with identical signatures.
	unsigned int num;
	// Signature represents return and argument types.
	BaseSignature *signature;

#ifdef EMSCRIPTEN
	const char *emSignature;
#endif
};

} // namespace
