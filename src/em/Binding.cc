// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#ifdef EMSCRIPTEN

#include <cstdlib>
#include <cstring>

#include "nbind/BindDefiner.h"

using namespace nbind;

extern "C" {
	extern void _nbind_register_endian(unsigned char byte);
	extern void _nbind_register_pool(unsigned int pageSize, unsigned int *usedPtr, unsigned char *rootPtr, unsigned char **pagePtr);
	extern void _nbind_register_method_getter_setter_id(unsigned int methodID, unsigned int getterID, unsigned int setterID);
	extern void _nbind_register_primitive(  TYPEID typeID, unsigned int size, unsigned char flag);
	extern void _nbind_register_type(       TYPEID typeID,    const char *name);
	extern void _nbind_register_class(const TYPEID *typeList, const char *name);
	extern void _nbind_register_destructor( TYPEID classType, funcPtr func);
	extern void _nbind_register_constructor(TYPEID classType, const char **policies, const TYPEID *types, unsigned int typeCount, funcPtr func, funcPtr ptrValue);
	extern void _nbind_register_function(   TYPEID classType, const char **policies, const TYPEID *types, unsigned int typeCount, funcPtr func, const char *name,
		unsigned int num, funcPtr direct);
	extern void _nbind_register_method(     TYPEID classType, const char **policies, const TYPEID *types, unsigned int typeCount, funcPtr func, const char *name,
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
	uint32_t endianTest = 0x01020304;

	_nbind_register_endian(*(uint8_t *)&endianTest);

	_nbind_register_pool(Pool::pageSize, &Pool::used, Pool::rootPage, &Pool::page);

	const void **primitiveData = getPrimitiveList();
	const uint32_t *sizePtr = static_cast<const uint32_t *>(primitiveData[1]);
	const uint8_t *flagPtr = static_cast<const uint8_t *>(primitiveData[2]);

	for(const TYPEID *type = static_cast<const TYPEID *>(primitiveData[0]); *type; ++type) {
		_nbind_register_primitive(*type, *(sizePtr++), *(flagPtr++));
	}

	for(const void **type = getNamedTypeList(); *type; type += 2) {
		_nbind_register_type(
			static_cast<TYPEID>(type[0]),
			static_cast<const char *>(type[1])
		);
	}

	_nbind_register_type(Typer<cbOutput::CreateValue>::makeID(), "_nbind_new");

	// Register all classes before any functions or methods,
	// so they'll have class type IDs available.

	auto &classList = getClassList();

	for(auto pos = classList.begin(); pos != classList.end(); ++pos ) {
		auto *bindClass = *pos;

		// Avoid registering the same class twice.
		if(!bindClass || bindClass->isReady()) {
			*pos = nullptr;
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
			signature->getPolicies(),
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
		if(!bindClass) continue;

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
						signature->getPolicies(),
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
						signature->getPolicies(),
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
						signature->getPolicies(),
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
	method(queryType);

	method(lalloc);
	method(lreset);
}

#endif
