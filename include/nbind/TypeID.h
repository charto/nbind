// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// These must match JavaScript enum StructureType in Type.ts

enum class StructureType : unsigned char {
	none = 0,
	constant,
	pointer,
	reference,
	rvalue,
	shared,
	unique,
	vector,
	array,
	callback,
	max
};

// Type ID system, based on Embind.

typedef const void *TYPEID;

// Type ID generator.

template<typename ArgType>
struct Typer {
	// Reserve a single byte of memory to uniquely represent a type.
	// The address of this byte is a unique type-specific constant.
	static const struct SpecType {
		const StructureType placeholderFlag;
	} spec;

	static NBIND_CONSTEXPR TYPEID makeID() {
		return(&spec.placeholderFlag);
	}
};

// Linkage for placeholder bytes representing types.
template<typename ArgType>
const typename Typer<ArgType>::SpecType Typer<ArgType>::spec = {
	StructureType :: none
};

// Parameterized types

struct ParamStructure {
	const StructureType placeholderFlag;
	const TYPEID target;
};

#define NBIND_TYPER_PARAM(Type, flag)           \
template<typename ArgType>                      \
struct Typer<Type> {                            \
	static const ParamStructure spec;           \
	                                            \
	static NBIND_CONSTEXPR TYPEID makeID() {    \
		return(&spec.placeholderFlag);          \
	}                                           \
};                                              \
                                                \
template<typename ArgType>                      \
const ParamStructure Typer<Type>::spec = {      \
	StructureType :: flag,                      \
	Typer<ArgType>::makeID()                    \
}

NBIND_TYPER_PARAM(const ArgType, constant);
NBIND_TYPER_PARAM(ArgType *, pointer);
NBIND_TYPER_PARAM(ArgType &, reference);

// Convert a list of types in the template argument into an array of type IDs.

template<typename... Args>
const TYPEID *listTypes() {
	static NBIND_CONSTEXPR TYPEID typeList[] = { Typer<Args>::makeID()... };
	return(typeList);
}

} // namespace
