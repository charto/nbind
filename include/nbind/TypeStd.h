// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <string>
#include <vector>
#include <array>

namespace nbind {

	enum class StructureType {
		raw = 0,
		vector = 1,
		array = 2
	};

	typedef struct {
		const unsigned char placeholderFlag;
		const TYPEID member;
	} VectorStructure;

	typedef struct {
		const unsigned char placeholderFlag;
		const TYPEID member;
		const size_t length;
	} ArrayStructure;

	template<typename MemberType>
	struct Typer<std::vector<MemberType>> {
		static const VectorStructure spec;

		static NBIND_CONSTEXPR TYPEID makeID() {
			return(&spec.placeholderFlag);
		}
	};

	template<typename MemberType>
	const VectorStructure Typer<std::vector<MemberType>>::spec = {
		static_cast<unsigned char>(StructureType :: vector),
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
		static_cast<unsigned char>(StructureType :: array),
		Typer<MemberType>::makeID(),
		size
	};

} // namespace
