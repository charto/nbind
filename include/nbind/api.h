// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

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

// Macro to report an error when exceptions are not available.

#define NBIND_ERR(message) nbind::Status::setError(message)

#ifdef BUILDING_NODE_EXTENSION
#include "nbind/v8/BindingType.h"
#elif EMSCRIPTEN
#include "nbind/em/BindingType.h"
#endif

#include "TypeID.h"
