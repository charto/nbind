// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#ifdef EMSCRIPTEN

#include <cstdlib>
#include <cstring>

#include "nbind/BindDefiner.h"

using namespace nbind;

extern "C" {
	extern void _nbind_register_pool(unsigned int pageSize, unsigned int *usedPtr, unsigned char *rootPtr, unsigned char **pagePtr);
	extern void _nbind_register_method_getter_setter_id(unsigned int methodID, unsigned int getterID, unsigned int setterID);
	extern void _nbind_register_types(void **data);
	extern void _nbind_register_type(       TYPEID typeID,    const char *name);
	extern void _nbind_register_class(const TYPEID *typeList, const char *name);
	extern void _nbind_register_destructor( TYPEID classType, funcPtr func);
	extern void _nbind_register_constructor(TYPEID classType, const TYPEID *types, unsigned int typeCount, funcPtr func, funcPtr ptrValue);
	extern void _nbind_register_function(   TYPEID classType, const TYPEID *types, unsigned int typeCount, funcPtr func, const char *name,
		unsigned int num, funcPtr direct);
	extern void _nbind_register_method(     TYPEID classType, const TYPEID *types, unsigned int typeCount, funcPtr func, const char *name,
		unsigned int num, unsigned int methodType
	);
	void nbind_init();
}

typedef BaseSignature::Type SigType;

unsigned int Pool::used = 0;
unsigned char *Pool::rootPage = new unsigned char[Pool::pageSize];
unsigned char *Pool::page = nullptr;

/** Simple linear allocator. Return consecutive blocks on a root page.
  * Allocate blocks in a linked list on the heap if they're too large. */

unsigned int NBind :: lalloc(size_t size) {
	// Round size up to a multiple of 8 bytes (size of a double)
	// to align pointers allocated later.
	size = (size + 7) & ~7;

	if(size > Pool::pageSize / 2 || size > Pool::pageSize - Pool::used) {
		// TODO: make sure the memory is properly aligned!
		unsigned char *page = new unsigned char[size + 8];
		if(!page) return(0); // Out of memory, should throw?
		*(unsigned char **)page = Pool::page;
		Pool::page = page;
		return(reinterpret_cast<unsigned int>(page) + 8);
	} else {
		Pool::used += size;
		return(reinterpret_cast<unsigned int>(Pool::rootPage) + Pool::used);
	}
}

/** Reset linear allocator to a previous state.
  * Set root page used size and free blocks allocated since the old state. */

void NBind :: lreset(unsigned int used, unsigned int page) {
	while(Pool::page != reinterpret_cast<unsigned char *>(page)) {
		unsigned char *topPage = Pool::page;
		Pool::page = *(unsigned char **)topPage;
		delete topPage;
	}

	Pool::used = used;
}

PoolRestore :: PoolRestore() : used(Pool::used), page(Pool::page) {}

PoolRestore :: ~PoolRestore() {
	NBind::lreset(used, reinterpret_cast<unsigned int>(page));
}

static void initModule() {
	_nbind_register_pool(Pool::pageSize, &Pool::used, Pool::rootPage, &Pool::page);

	_nbind_register_type(Typer<void>::makeID(), "void");
	_nbind_register_type(Typer<bool>::makeID(), "bool");
	_nbind_register_type(Typer<std::string>::makeID(), "std::string");
	_nbind_register_type(Typer<cbOutput::CreateValue>::makeID(), "_nbind_new");

	_nbind_register_types(defineTypes<
		unsigned char,  signed char,    char,
		unsigned short, signed short,
		unsigned int,   signed int,
		unsigned long,  signed long,

		float, double,

		unsigned char *, const unsigned char *,
		signed   char *, const signed   char *,
		         char *, const          char *
	>());

	_nbind_register_type(Typer<cbFunction &>::makeID(), "cbFunction &");

	// Register all classes before any functions, so they'll have type information ready.

	for(auto *bindClass : getClassList()) {
		// Avoid registering the same class twice.
		if(bindClass->isReady()) {
			bindClass->setDuplicate();
			continue;
		}

		bindClass->init();

		_nbind_register_class(bindClass->getTypes(), bindClass->getName());
	}

	_nbind_register_method_getter_setter_id(
		static_cast<unsigned int>(SigType::method),
		static_cast<unsigned int>(SigType::getter),
		static_cast<unsigned int>(SigType::setter)
	);

	// Register all functions.

	for(auto *bindClass : getClassList()) {
		if(bindClass->isDuplicate()) continue;

		TYPEID id = bindClass->getTypes()[0];

		_nbind_register_destructor(id, bindClass->getDeleter());

		for(auto &func : bindClass->getMethodList()) {

			BaseSignature *signature = func.getSignature();

			if(signature == nullptr) {
				continue;
			}

			switch(signature->getType()) {
				case SigType::method:
				case SigType::getter:
				case SigType::setter:

					_nbind_register_method(
						id,
						signature->getTypeList(),
						signature->getArity() + 1,
						signature->getCaller(),
						func.getName(),
						func.getNum(),
						static_cast<unsigned int>(signature->getType())
					);

					break;

				case SigType::function:

					_nbind_register_function(
						id,
						signature->getTypeList(),
						signature->getArity() + 1,
						signature->getCaller(),
						func.getName(),
						func.getNum(),
						func.getPtr()
					);

					break;

				case SigType::constructor:

					_nbind_register_constructor(
						id,
						signature->getTypeList(),
						signature->getArity() + 1,
						signature->getCaller(),
						signature->getValueConstructor()
					);

					break;
			}
		}
	}
}

void nbind_init(void) {
	initModule();
}

#include "nbind/nbind.h"

NBIND_CLASS(NBind) {
	construct<>();

	method(bind_value);
	method(lalloc);
	method(lreset);
}

#endif
