// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

class NBindType {

public:

	explicit NBindType(TYPEID id);
	NBindType (const NBindType &other);
	NBindType (NBindType &&other);
	~NBindType();

	NBindType &operator=(const NBindType &) = delete;
	NBindType &operator=(NBindType &&) = delete;

	const void *getStructure() const;
	StructureType getStructureType() const;
	const char *toString();

private:

	union {
		TYPEID id;
		const void *structure;
		const StructureType *structureType;
	};

	const char *name;

};

class cbFunction;

class NBind {

public:

	static void bind_value(const char *name, cbFunction &func);

	static void reflect(
		cbFunction &outPrimitive,
		cbFunction &outType,
		cbFunction &outClass,
		cbFunction &outMethod
	);

	static void queryType(NBindType type, cbFunction &outTypeDetail);

};

} // namespace

#define nbind_debug()
