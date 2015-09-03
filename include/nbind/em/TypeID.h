// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Function signature string generation.

template<typename ArgType> constexpr char emMangle();

template<> constexpr char emMangle<int>() {return('i');}
template<> constexpr char emMangle<void>() {return('v');}
template<> constexpr char emMangle<float>() {return('f');}
template<> constexpr char emMangle<double>() {return('d');}

template<typename... Args>
const char* buildEmSignature() {
	static constexpr char signature[] = { emMangle<Args>()..., '\0' };

	return(signature);
}

template<typename ArgType> struct EmMangleMap { typedef int type; };
template<> struct EmMangleMap<void> { typedef void type; };
template<> struct EmMangleMap<float> { typedef float type; };
template<> struct EmMangleMap<double> { typedef double type; };

} // namespace
