// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#ifdef EMSCRIPTEN

#include <cstdlib>
#include <cstring>

#include "nbind/BindDefiner.h"

using namespace nbind;

extern "C" {
	extern void _nbind_register_pool(unsigned int pageSize, unsigned int *usedPtr, unsigned char *rootPtr, unsigned char **pagePtr);
	extern void _nbind_register_method_getter_setter_id(unsigned int methodID, unsigned int getterID, unsigned int setterID);
	extern void _nbind_register_types(const void **data);
	extern void _nbind_register_type(       TYPEID typeID,    const char *name);
	extern void _nbind_register_class(const TYPEID *typeList, const char *name);
	extern void _nbind_register_destructor( TYPEID classType, funcPtr func);
	extern void _nbind_register_constructor(TYPEID classType, const TYPEID *types, unsigned int typeCount, funcPtr func, funcPtr ptrValue);
	extern void _nbind_register_function(   TYPEID classType, const TYPEID *types, unsigned int typeCount, funcPtr func, const char *name,
		unsigned int num, funcPtr direct);
	extern void _nbind_register_method(     TYPEID classType, const TYPEID *types, unsigned int typeCount, funcPtr func, const char *name,
		unsigned int num, unsigned int methodType
	);
}

typedef BaseSignature::Type SigType;

unsigned int Pool::used = 0;
unsigned char *Pool::rootPage = new unsigned char[Pool::pageSize];
unsigned char *Pool::page = nullptr;

/** Simple linear allocator. Return consecutive blocks on a constant-sized
  * root page. Blocks that don't fit are allocated in a linked list on the heap.
  * The memory is used like a stack to pass function parameters and return
  * values.
  *
  * Alloca could be used instead for parameters of calls to JavaScript,
  * but not for return values of calls from JavaScript because they may get
  * corrupted when the invoker function's stack frame is freed.
  * It's easier to use a single allocator for both cases.
  *
  * Returned bytes must be aligned to 8 bytes to support storing doubles.
  * Asm.js can't handle unaligned access. */

unsigned int NBind :: lalloc(size_t size) {
	// Round size up to a multiple of 8 bytes (size of a double)
	// to align pointers allocated later.
	size = (size + 7) & ~7;

	// Check if block won't fit on the root page or is bigger than half its size.
	// The latter condition avoids filling the root page with large blocks,
	// unnecessarily slowing down allocation of any later small blocks.

	if(size > Pool::pageSize / 2 || size > Pool::pageSize - Pool::used) {
		// Allocate a block on the heap.

		// TODO: make sure the memory allocated here is properly aligned!
		// Allocate a new block in the heap and reserve 8 bytes
		// (to maintain alignment) for a header pointing to the previous block.

		unsigned char *page = new unsigned char[size + 8];
		if(!page) return(0); // Out of memory, should throw?

		// Store address of previous block at the start of the new block.
		*(unsigned char **)page = Pool::page;

		// Make the new block the current one.
		Pool::page = page;

		return(reinterpret_cast<unsigned int>(page) + 8);
	} else {
		// Allocate a block on the root page by simply growing the used byte count.
		Pool::used += size;

		return(reinterpret_cast<unsigned int>(Pool::rootPage) + Pool::used);
	}
}

/** Reset linear allocator to a previous state, effectively to free
  * a stack frame. Set root page used byte count to match the earlier state
  * and free all blocks allocated since then. */

void NBind :: lreset(unsigned int used, unsigned int page) {
	// Free all blocks allocated since the earlier state.

	while(Pool::page != reinterpret_cast<unsigned char *>(page)) {
		unsigned char *topPage = Pool::page;
		Pool::page = *(unsigned char **)topPage;
		delete topPage;
	}

	Pool::used = used;
}

/** RAII constructor to store the lalloc pool state. */

PoolRestore :: PoolRestore() : used(Pool::used), page(Pool::page) {}

/** RAII destructor to restore the lalloc pool state. */

PoolRestore :: ~PoolRestore() {
	NBind::lreset(used, reinterpret_cast<unsigned int>(page));
}

static void initModule() {
	_nbind_register_pool(Pool::pageSize, &Pool::used, Pool::rootPage, &Pool::page);

	_nbind_register_type(Typer<void>::makeID(), "void");
	_nbind_register_type(Typer<bool>::makeID(), "bool");
	_nbind_register_type(Typer<std::string>::makeID(), "std::string");
	_nbind_register_type(Typer<cbOutput::CreateValue>::makeID(), "_nbind_new");
	_nbind_register_type(Typer<uint64_t>::makeID(), "Int64");
	_nbind_register_type(Typer<int64_t>::makeID(), "Int64");

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

	// Register all classes before any functions or methods,
	// so they'll have class type IDs available.

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

	for(auto &func : getFunctionList()) {
		const BaseSignature *signature = func.getSignature();

		_nbind_register_function(
			0,
			signature->getTypeList(),
			signature->getArity() + 1,
			signature->getCaller(),
			func.getName(),
			func.getNum(),
			func.getPtr()
		);
	}

	// Register all methods.

	for(auto *bindClass : getClassList()) {
		if(bindClass->isDuplicate()) continue;

		TYPEID id = bindClass->getTypes()[0];

		_nbind_register_destructor(id, bindClass->getDeleter());

		for(auto &func : bindClass->getMethodList()) {

			const BaseSignature *signature = func.getSignature();

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

extern "C" {
	void nbind_init(void) {
		initModule();
	}
}

#include "nbind/nbind.h"

NBIND_CLASS(NBind) {
	construct<>();

	method(bind_value);
	method(reflect);

	method(lalloc);
	method(lreset);
}

#endif
