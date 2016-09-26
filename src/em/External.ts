// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file allows C++ to hold references to arbitrary JavaScript objects.
// Each object is stored in a JavaScript array, and C++ receives its index.
// C++ can then call JavaScript methods and refer to the object by index.

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

	export var externalList: (External<any> | number)[] = [0];

	// Head of free list for recycling available slots in the externals list.
	let firstFreeExternal = 0;

	export class External<Member> {
		constructor(data: Member) {
			this.data = data;
		}

		// Store this external in a JavaScript array and return its index
		// creating a reference that can be passed to C++.

		register() {
			let num = firstFreeExternal;

			if(num) {
				firstFreeExternal = externalList[num] as number;
			} else num = externalList.length;

			externalList[num] = this;

			return(num);
		}

		// Called by C++ side destructor through unregisterExternal
		// to free any related JavaScript resources.

		free?(): void

		refCount = 1;
		data: any;
	}

	export function unregisterExternal(num: number) {
		const external = externalList[num] as External<any>;
		if(external.free) external.free();

		externalList[num] = firstFreeExternal;
		firstFreeExternal = num;
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}

@exportLibrary
class nbind { // tslint:disable-line:class-name

	@dep('_nbind')
	static _nbind_reference_external(num: number) {
		++(_nbind.externalList[num] as _nbind.External<any>).refCount;
	}

	@dep('_nbind')
	static _nbind_free_external(num: number) {
		if(--(_nbind.externalList[num] as _nbind.External<any>).refCount == 0) {
			_nbind.unregisterExternal(num);
		}
	}

}
