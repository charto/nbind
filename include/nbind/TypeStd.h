// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#include <string>
#include <vector>
#include <array>

namespace nbind {

	struct ArrayType {
		const char placeholder;
		const TYPEID member;
		size_t size;
	};

	template<typename ArgType, size_t size>
	struct Typer<std::array<ArgType, size>> {
		static NBIND_CONSTEXPR TYPEID makeID() {
			return(DataTypeStore<1, struct ArrayType, TypeMembers<ArgType>, TypeLimits<size>>::get());
		}
	};

// Specialize type ID generator for vector types.

template<typename ArgType>
struct Typer<std::vector<ArgType>> {
	static NBIND_CONSTEXPR TYPEID makeID() {
		return(DataTypeStore<2, struct StructuredType, TypeMembers<ArgType>, TypeLimits<>>::get());
	}
};

} // namespace
