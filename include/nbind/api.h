// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

// Macro to report an error when exceptions are not available.

#define NBIND_ERR(message) nbind::Status::setError(message)

// Visual Studio 2015 doesn't accept some things as constexpr
// while GCC and Clang do, so change them to const only if needed.

#ifdef _MSC_VER
#define NBIND_CONSTEXPR const
#else
#define NBIND_CONSTEXPR constexpr
#endif

namespace nbind {

typedef void (*funcPtr)();

class Status {

public:

	static inline const char *getError() {return(message);}
	static inline void clearError() {Status::message = nullptr;}
	static inline void setError(const char *message) {
		if(!Status::message) Status::message = message;
	}

private:

	static const char *message;

};

} // namespace

#ifdef BUILDING_NODE_EXTENSION
#include "nbind/v8/BindWrapper.h"
#include "nbind/v8/BindingType.h"
#include "nbind/v8/BindingStd.h"
#elif EMSCRIPTEN
#include "nbind/em/BindingType.h"
#endif

#include "TypeID.h"
