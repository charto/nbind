// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

#define NBIND_MULTIMETHOD(name, args, ...) definer.overloaded args ().method(name, ## __VA_ARGS__)

#include "noconflict.h"

#define function NBIND_FUNCTION
#define multifunction NBIND_MULTIFUNCTION
#define method(...) NBIND_EXPAND(NBIND_METHOD(__VA_ARGS__))
#define inherit(...) NBIND_INHERIT(__VA_ARGS__)
#define args NBIND_ARGS
#define multimethod NBIND_MULTIMETHOD
#define construct NBIND_CONSTRUCT
#define field(...) NBIND_FIELD(__VA_ARGS__)
#define getter NBIND_GETTER
#define getset NBIND_GETSET
