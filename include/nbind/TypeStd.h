// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <array>
#include <functional>

namespace nbind {

NBIND_TYPER_PARAM(std::shared_ptr<ArgType>, shared);
NBIND_TYPER_PARAM(std::unique_ptr<ArgType>, unique);
NBIND_TYPER_PARAM(std::vector<ArgType>, vector);

typedef struct {
	const StructureType placeholderFlag;
	const TYPEID member;
	const size_t length;
} ArrayStructure;

template<typename MemberType, size_t size>
struct Typer<std::array<MemberType, size>> {
	static const ArrayStructure spec;

	static NBIND_CONSTEXPR TYPEID makeID() {
		return(&spec.placeholderFlag);
	}
};

template<typename MemberType, size_t size>
const ArrayStructure Typer<std::array<MemberType, size>>::spec = {
	StructureType :: array,
	Typer<MemberType>::makeID(),
	size
};

//template<typename ReturnType, typename... Args>
template <int argCount>
struct CallbackStructure {
	const StructureType placeholderFlag;
	const int arity;
	const TYPEID returnType;
	const TYPEID argType[argCount];
};

template<typename ReturnType, typename... Args>
struct Typer<std::function<ReturnType (Args...)>> {
	static const CallbackStructure<sizeof...(Args)> spec;

	static NBIND_CONSTEXPR TYPEID makeID() {
		return(&spec.placeholderFlag);
	}
};

template<typename ReturnType, typename... Args>
const CallbackStructure<sizeof...(Args)> Typer<std::function<ReturnType (Args...)>>::spec = {
	StructureType :: callback,
	sizeof...(Args),
	Typer<ReturnType>::makeID(),
	{ Typer<Args>::makeID()... }
};

} // namespace
