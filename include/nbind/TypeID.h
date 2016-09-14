// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// These must match JavaScript enum StructureType in common.ts

enum class StructureType : unsigned char {
	raw = 0,
	constant,
	pointer,
	reference,
	rvalue,
	vector,
	array
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
	StructureType :: raw
};

// Parameterized types

typedef struct {
	const StructureType placeholderFlag;
	const TYPEID target;
} ParamStructure;

// Const types

template<typename ArgType>
struct Typer<const ArgType> {
	static const ParamStructure spec;

	static NBIND_CONSTEXPR TYPEID makeID() {
		return(&spec.placeholderFlag);
	}
};

template<typename ArgType>
const ParamStructure Typer<const ArgType>::spec = {
	StructureType :: constant,
	Typer<ArgType>::makeID()
};

// Pointers

template<typename TargetType>
struct Typer<TargetType *> {
	static const ParamStructure spec;

	static NBIND_CONSTEXPR TYPEID makeID() {
		return(&spec.placeholderFlag);
	}
};

template<typename TargetType>
const ParamStructure Typer<TargetType *>::spec = {
	StructureType :: pointer,
	Typer<TargetType>::makeID()
};

// References

template<typename TargetType>
struct Typer<TargetType &> {
	static const ParamStructure spec;

	static NBIND_CONSTEXPR TYPEID makeID() {
		return(&spec.placeholderFlag);
	}
};

template<typename TargetType>
const ParamStructure Typer<TargetType &>::spec = {
	StructureType :: reference,
	Typer<TargetType>::makeID()
};

// Convert a list of types in the template argument into an array of type IDs.

template<typename... Args>
const TYPEID *listTypes() {
	static NBIND_CONSTEXPR TYPEID typeList[] = { Typer<Args>::makeID()... };
	return(typeList);
}

} // namespace
