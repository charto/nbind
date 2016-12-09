// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <cstdlib>

#include "nbind/api.h"

using namespace nbind;

// Convert getter names like "getFoo" into property names like "foo".
// This could be so much more concisely written with regexps...

const char *nbind :: stripGetterPrefix(const char *name, char *&nameBuf) {
	if(
		strlen(name) <= 3 ||
		(name[0] != 'G' && name[0] != 'g') ||
		name[1] != 'e' ||
		name[2] != 't'
	) return(name);

	char c = name[3];

	// "Get_foo", "get_foo" => Remove 4 first characters.
	if(c == '_') return(name + 4);

	// "Getfoo", "getfoo" => Remove 3 first characters.
	if(c >= 'a' && c <= 'z') return(name + 3);

	if(c >= 'A' && c <= 'Z') {
		// "GetFOO", "getFOO" => Remove first 3 characters.
		if(name[4] >= 'A' && name[4] <= 'Z') return(name + 3);
	} else return(name);

	// "GetFoo", "getFoo" => Remove 3 first characters,
	// make a modifiable copy and lowercase first letter.

	if(nameBuf != nullptr) free(nameBuf);
	nameBuf = strdup(name + 3);

	if(nameBuf != nullptr) {
		nameBuf[0] = c + ('a' - 'A');
		return(nameBuf);
	}

	// Memory allocation failed.
	// The world will soon end anyway, so just declare
	// the getter without stripping the "get" prefix.

	return(name);
}

NBindID :: NBindID(TYPEID id) : id(id), name(nullptr) {}

NBindID :: NBindID(const NBindID &other) :
	id(other.id), name(other.name ? strdup(other.name) : nullptr) {}

NBindID :: NBindID(NBindID &&other) : id(other.id), name(other.name) {
	other.name = nullptr;
}

NBindID :: ~NBindID() {
	if(name) delete(name);
	name = nullptr;
}

const void *NBindID :: getStructure() const {
	return(structure);
}

StructureType NBindID :: getStructureType() const {
	return(*structureType);
}

const char *NBindID :: toString() {
	if(!name) {
		static const char *alphabet = "0123456789abcdef";

		char *newName = new char[sizeof(id) * 2 + 1];
		unsigned int pos = sizeof(id) * 2;
		uintptr_t code = reinterpret_cast<uintptr_t>(id);

		newName[pos] = 0;

		while(pos--) {
			newName[pos] = alphabet[code & 15];
			code >>= 4;
		}

		name = newName;
	}

	return(name);
}

#include "nbind/nbind.h"

#ifndef NODE_USE_NAPI

NBIND_CLASS(NBind) {
	construct<>();

	method(bind_value);
	method(reflect);
	method(queryType);
}

NBIND_CLASS(NBindID) {
	method(toString);
}

#endif
