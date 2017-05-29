// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#ifdef EMSCRIPTEN

#include <cstdlib>
#include <cstring>

#include "nbind/BindDefiner.h"

using namespace nbind;

extern "C" {
	extern void _nbind_register_pool(unsigned int pageSize, unsigned int *usedPtr, unsigned char *rootPtr, unsigned char **pagePtr);
	extern void _nbind_register_primitive(TYPEID typeID, unsigned int size, unsigned char flag);
	extern void _nbind_register_type(TYPEID typeID, const char *name);
	extern void _nbind_register_class(const TYPEID *typeList,
		const char **policies, const TYPEID *superList, void *(**upcastList)(void *),
		unsigned int superCount,
		funcPtr destructor,
		const char *name
	);
	extern void _nbind_register_function(TYPEID boundID,
		const char **policies, const TYPEID *types, unsigned int typeCount,
		funcPtr func, funcPtr direct, unsigned int signatureType,
		const char *name, unsigned int num, unsigned int flags
	);
	extern void _nbind_finish();
}

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

uintptr_t NBind :: lalloc(size_t size) {
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

		return(reinterpret_cast<uintptr_t>(page) + 8);
	} else {
		uintptr_t result = reinterpret_cast<uintptr_t>(Pool::rootPage) + Pool::used;

		// Allocate a block on the root page by simply growing the used byte count.
		Pool::used += size;

		return(result);
	}
}

/** Reset linear allocator to a previous state, effectively to free
  * a stack frame. Set root page used byte count to match the earlier state
  * and free all blocks allocated since then. */

void NBind :: lreset(unsigned int used, uintptr_t page) {
	// Free all blocks allocated since the earlier state.

	while(Pool::page != reinterpret_cast<unsigned char *>(page)) {
		unsigned char *topPage = Pool::page;
		Pool::page = *(unsigned char **)topPage;
		delete topPage;
	}

	Pool::used = used;
}

NBindID :: NBindID(TYPEID id) : id(id) {}
NBindID :: NBindID(uintptr_t ptr) : id(reinterpret_cast<TYPEID>(ptr)) {}

const void *NBindID :: getStructure() const {
	return(structure);
}

StructureType NBindID :: getStructureType() const {
	return(*structureType);
}

void NBindID :: toJS(cbOutput output) const {
	output(reinterpret_cast<uintptr_t>(id));
}

/** RAII constructor to store the lalloc pool state. */

PoolRestore :: PoolRestore() : used(Pool::used), page(Pool::page) {}

/** RAII destructor to restore the lalloc pool state. */

PoolRestore :: ~PoolRestore() {
	NBind::lreset(used, reinterpret_cast<unsigned int>(page));
}

typedef BaseSignature :: SignatureType SignatureType;

static void initModule() {
	_nbind_register_pool(Pool::pageSize, &Pool::used, Pool::rootPage, &Pool::page);

	const void **primitiveData = getPrimitiveList();
	const uint8_t *sizePtr = static_cast<const uint8_t *>(primitiveData[1]);
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

	for(auto *bindClass : classList) bindClass->unvisit();

	auto posPrev = classList.before_begin();
	auto pos = classList.begin();

	while(pos != classList.end()) {
		auto *bindClass = *pos++;

		// Avoid registering the same class twice.
		if(bindClass->isVisited()) {
			classList.erase_after(posPrev);
			continue;
		}

		bindClass->visit();
		++posPrev;

		const unsigned int superCount = bindClass->getSuperClassCount();
		TYPEID superIdList[superCount];
		TYPEID *superPtr = superIdList;
		void *(*upcastList[superCount])(void *);
		auto *upcastPtr = upcastList;

		for(auto &spec : bindClass->getSuperClassList()) {
			*superPtr++ = spec.superClass.getTypes()[0];
			*upcastPtr++ = spec.upcast;
		}

		_nbind_register_class(
			bindClass->getTypes(),
			bindClass->getPolicies(),
			superIdList,
			upcastList,
			bindClass->getSuperClassCount(),
			bindClass->getDeleter(),
			bindClass->getName()
		);
	}

	// Register all functions.

	for(auto &func : getFunctionList()) {
		const BaseSignature *signature = func.getSignature();

		_nbind_register_function(
			0,
			signature->getPolicies(),
			signature->getTypeList(),
			signature->getArity() + 1,
			signature->getCaller(),
			func.getPtr(),
			static_cast<unsigned int>(signature->getType()),
			func.getName(),
			func.getNum(),
			static_cast<unsigned int>(func.getFlags())
		);
	}

	// Register all methods.

	for(auto *bindClass : getClassList()) {
		if(!bindClass) continue;

		TYPEID id = bindClass->getTypes()[0];

		for(auto &func : bindClass->getMethodList()) {

			const BaseSignature *signature = func.getSignature();

			if(signature == nullptr) {
				continue;
			}

			switch(signature->getType()) {
				case SignatureType :: none:

					// Impossible!
					abort();

				case SignatureType :: method:
				case SignatureType :: getter:
				case SignatureType :: setter:

					_nbind_register_function(
						id,
						signature->getPolicies(),
						signature->getTypeList(),
						signature->getArity() + 1,
						signature->getCaller(),
						nullptr,
						static_cast<unsigned int>(signature->getType()),
						func.getName(),
						func.getNum(),
						static_cast<unsigned int>(func.getFlags())
					);

					break;

				case SignatureType :: func:

					_nbind_register_function(
						id,
						signature->getPolicies(),
						signature->getTypeList(),
						signature->getArity() + 1,
						signature->getCaller(),
						func.getPtr(),
						static_cast<unsigned int>(signature->getType()),
						func.getName(),
						func.getNum(),
						static_cast<unsigned int>(func.getFlags())
					);

					break;

				case SignatureType :: construct:

					_nbind_register_function(
						id,
						signature->getPolicies(),
						signature->getTypeList(),
						signature->getArity() + 1,
						signature->getCaller(),
						signature->getValueConstructor(),
						static_cast<unsigned int>(signature->getType()),
						nullptr, 0, 0
					);

					break;
			}
		}
	}

	// Set up inheritance.

	_nbind_finish();
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

NBIND_CLASS(NBindID) {
	construct<uintptr_t>();
}

#endif
