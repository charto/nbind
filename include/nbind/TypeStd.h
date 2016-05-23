// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <string>
#include <vector>
#include <array>

namespace nbind {

	template<typename MemberType>
	struct Typer<std::vector<MemberType>> {
		static const struct SpecType {
			const char placeholderFlag;
			const TYPEID member;
		} spec;

		static NBIND_CONSTEXPR TYPEID makeID() {
			return(&spec.placeholderFlag);
		}
	};

	template<typename MemberType>
	const typename Typer<std::vector<MemberType>>::SpecType
		Typer<std::vector<MemberType>>::spec = {
			1,
			Typer<MemberType>::makeID()
		};

	template<typename MemberType, size_t size>
	struct Typer<std::array<MemberType, size>> {
		static const struct SpecType {
			const char placeholderFlag;
			const TYPEID member;
			const size_t length;
		} spec;

		static NBIND_CONSTEXPR TYPEID makeID() {
			return(&spec.placeholderFlag);
		}
	};

	template<typename MemberType, size_t size>
	const typename Typer<std::array<MemberType, size>>::SpecType
		Typer<std::array<MemberType, size>>::spec = {
			2,
			Typer<MemberType>::makeID(),
			size
		};

} // namespace
