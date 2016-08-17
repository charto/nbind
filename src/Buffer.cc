// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#include "nbind/nbind.h"

using namespace nbind;

Buffer :: Buffer(unsigned char *ptr, size_t len) : ptr(ptr), len(len) {}

unsigned char *Buffer :: data() { return(ptr); }

size_t Buffer :: length() { return(len); }
