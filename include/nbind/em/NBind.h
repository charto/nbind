// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class cbOutput;

class NBindID {

public:

	explicit NBindID(TYPEID id);
	explicit NBindID(uintptr_t ptr);

	const void *getStructure() const;
	StructureType getStructureType() const;
	void toJS(cbOutput output) const;

private:

	union {
		TYPEID id;
		const void *structure;
		const StructureType *structureType;
	};

};

class External;

template <typename DefaultReturnType>
class cbWrapper;

typedef cbWrapper<void> cbFunction;

class NBind {

public:

	static void bind_value(const char *name, cbFunction &func);

	static void reflect(
		cbFunction &outPrimitive,
		cbFunction &outType,
		cbFunction &outClass,
		cbFunction &outSuper,
		cbFunction &outMethod
	);

	static External queryType(NBindID type, cbFunction &outTypeDetail);

	static uintptr_t lalloc(size_t size);
	static void lreset(unsigned int used, uintptr_t page);

};

} // namespace

extern "C" {
	extern void nbind_debug(void);
}
