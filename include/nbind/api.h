// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

// Check for C++11 support in the compiler.

#if defined(_MSC_VER)
	// Visual Studio 2015 has good enough support
	// even though it sets __cplusplus to 199711
#	if _MSC_VER < 1900
#		error nbind requires at least Visual Studio 2015.
#	endif
#elif (__cplusplus < 201103L)
#	error nbind requires a compiler with sufficient C++11 support. Maybe compile with -std=c++11 or similar?
#endif

// Macro to report an error when exceptions are not available.

#define NBIND_ERR(message) nbind::Status::setError(message)

// Visual Studio 2015 doesn't accept some things as constexpr
// while GCC and Clang do, so change them to const only if needed.

#if defined(_MSC_VER)
#	define NBIND_CONSTEXPR const
#else
#	define NBIND_CONSTEXPR constexpr
#endif

#include <utility>
#include <cstdint>
#include <cstring>

namespace nbind {

typedef void (*funcPtr)();

class Status {

public:

	static inline const char *getError() { return(message); }
	static inline void clearError() { Status::message = nullptr; }
	static inline void setError(const char *message) {
		if(!Status::message) Status::message = message;
	}

private:

	static const char *message;

};

} // namespace

#include "TypeID.h"
#include "TypeStd.h"

#if defined(BUILDING_NODE_EXTENSION)

#	include "v8/BindWrapper.h"
#	include "v8/BindingType.h"
#	include "v8/NBind.h"
#	include "v8/Callback.h"
#	include "v8/BindingStd.h"

#elif defined(EMSCRIPTEN)

#	include "em/BindingType.h"
#	include "signature/CallbackSignature.h"
#	include "em/NBind.h"
#	include "em/Pool.h"
#	include "em/Callback.h"
#	include "em/BindingStd.h"

#endif
