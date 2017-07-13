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
import { _nbind as _type } from './BindingType';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export const BindType = _type.BindType;
}

export namespace _nbind {

	// External JavaScript types are stored in a list,
	// so C++ code can find them by number.
	// A reference count allows storing them in C++ without leaking memory.
	// The first element is a dummy value just so that a valid index to
	// the list always tests as true (useful for the free list implementation).

	export let externalList: (External<any> | number)[] = [0];

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

		reference() { ++this.refCount; }

		dereference(this: External<any>, num: number) {
			if(--this.refCount == 0) {
				if(this.free) this.free();

				externalList[num] = firstFreeExternal;
				firstFreeExternal = num;
			}
		}

		// Called by C++ side destructor through unregisterExternal
		// to free any related JavaScript resources.

		free?(): void;

		refCount = 1;
		data: any;
	}

	function popExternal(num: number) {
		const obj = externalList[num] as External<any>;

		obj.dereference(num);

		return(obj.data);
	}

	function pushExternal(obj: any) {
		const external = new External(obj);

		external.reference();

		return(external.register());
	}

	export class ExternalType extends BindType {
		wireRead = popExternal;
		wireWrite = pushExternal;
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}

@exportLibrary
class nbind { // tslint:disable-line:class-name

	@dep('_nbind')
	static _nbind_reference_external(num: number) {
		(_nbind.externalList[num] as _nbind.External<any>).reference();
	}

	@dep('_nbind')
	static _nbind_free_external(num: number) {
		(_nbind.externalList[num] as _nbind.External<any>).dereference(num);
	}

}
