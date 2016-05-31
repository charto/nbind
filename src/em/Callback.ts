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
import {_nbind as _globals} from './Globals';
import {_nbind as _type} from './BindingType';
import {_nbind as _caller} from './Caller';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export var BindType = _type.BindType;
}

export namespace _nbind {

	export var readTypeIdList: typeof _globals.readTypeIdList;
	export var throwError: typeof _globals.throwError;

	export var makeJSCaller: typeof _caller.makeJSCaller;

	// List of invoker functions for all argument and return value combinations
	// seen so far.

	export var callbackSignatureList: _globals.Func[] = [];

	// Callbacks are stored in a list, so C++ code can find them by number.
	// A reference count allows storing them in C++ without leaking memory.
	// The first element is a dummy value just so that a valid index to
	// the list always tests as true (useful for the free list implementation).

	export var callbackList: _globals.Func[] = [null];
	export var callbackRefCountList: number[] = [0];

	// Free list for recycling available slots in the callback list.

	export var callbackFreeList: number[] = [];

	export function registerCallback(func: _globals.Func) {
		if(typeof(func) != 'function') _nbind.throwError('Type mismatch');

		const num = callbackFreeList.pop() || callbackList.length;

		callbackList[num] = func;
		callbackRefCountList[num] = 1;

		return(num);
	}

	export function unregisterCallback(num: number) {
		callbackList[num] = null;
		callbackFreeList.push(num);
	}

	export class CallbackType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		wireWrite = registerCallback;

		// Optional type conversion code
		// makeWireWrite = (expr: string) => '_nbind.registerCallback(' + expr + ')';
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}

@exportLibrary
class nbind { // tslint:disable-line:class-name

	@dep('_nbind')
	static _nbind_register_callback_signature(
		typeListPtr: number,
		typeCount: number
	) {
		const typeList = _nbind.readTypeIdList(typeListPtr, typeCount);
		const num = _nbind.callbackSignatureList.length;

		_nbind.callbackSignatureList[num] = _nbind.makeJSCaller(typeList);

		return(num);
	}

	@dep('_nbind')
	static _nbind_reference_callback(num: number) {
		++_nbind.callbackRefCountList[num];
	}

	@dep('_nbind')
	static _nbind_free_callback(num: number) {
		if(--_nbind.callbackRefCountList[num] == 0) _nbind.unregisterCallback(num);
	}

}
