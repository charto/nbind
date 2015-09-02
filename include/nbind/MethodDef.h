// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Storage format for method definitions.

class MethodDef {

public:

	enum class Type {function, method, getter, setter};

	MethodDef(Type type, const char *name, unsigned int num, funcPtr caller) :
		type(type), name(name), num(num), caller(caller) {}

	const char *getName() {return(name);}
	unsigned int getNum() {return(num);}
	funcPtr getCaller() {return(caller);}
	Type getType() {return(type);}

#ifdef EMSCRIPTEN
	void emInit(const char *emSignature) {
		this->emSignature = emSignature;
	}

	const char *getEmSignature() {
		return(emSignature);
	}
#endif

private:

	Type type;
	const char *name;
	// Index to distinguish between functions with identical signatures.
	unsigned int num;
	// Signature and its caller represent return and argument types.
	funcPtr caller;

#ifdef EMSCRIPTEN
	const char *emSignature;
#endif
};

} // namespace
