// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <string>
#include <vector>
#include <array>

namespace nbind {

	typedef struct {
		const StructureType placeholderFlag;
		const TYPEID member;
		const size_t length;
	} ArrayStructure;

	template<typename MemberType>
	struct Typer<std::vector<MemberType>> {
		static const ParamStructure spec;

		static NBIND_CONSTEXPR TYPEID makeID() {
			return(&spec.placeholderFlag);
		}
	};

	template<typename MemberType>
	const ParamStructure Typer<std::vector<MemberType>>::spec = {
		StructureType :: vector,
		Typer<MemberType>::makeID()
	};

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

} // namespace
