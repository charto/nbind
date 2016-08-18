// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles type conversion of JavaScript callback functions
// accessible from C++. See also Caller.ts

import {
	setEvil,
	prepareNamespace,
	exportLibrary,
	dep
} from 'emscripten-library-decorator';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {

	// External JavaScript types are stored in a list,
	// so C++ code can find them by number.
	// A reference count allows storing them in C++ without leaking memory.
	// The first element is a dummy value just so that a valid index to
	// the list always tests as true (useful for the free list implementation).

	export var externalList: any[] = [null];
	export var externalRefCountList: number[] = [0];

	// Free list for recycling available slots in the externals list.

	export var externalFreeList: number[] = [];

	export function registerExternal(obj: any) {
		const num = externalFreeList.pop() || externalList.length;

		externalList[num] = obj;
		externalRefCountList[num] = 1;

		return(num);
	}

	export function unregisterExternal(num: number) {
		externalList[num] = null;
		externalFreeList.push(num);
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}

@exportLibrary
class nbind { // tslint:disable-line:class-name

	@dep('_nbind')
	static _nbind_reference_external(num: number) {
		++_nbind.externalRefCountList[num];
	}

	@dep('_nbind')
	static _nbind_free_external(num: number) {
		if(--_nbind.externalRefCountList[num] == 0) _nbind.unregisterExternal(num);
	}

}
