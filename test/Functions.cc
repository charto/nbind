// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include <cstring>
#include <string>

int incrementInt(int x) { return(x + 1); }
int decrementInt(int x) { return(x - 1); }

#include "nbind/nbind.h"

#ifdef NBIND_GLOBAL

NBIND_GLOBAL() {
	function(incrementInt);
	function(decrementInt);
}

#endif
