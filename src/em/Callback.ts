// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles type conversion of JavaScript callback functions
// accessible from C++. See also Caller.ts

import {setEvil, prepareNamespace, defineHidden, exportLibrary, dep} from 'emscripten-library-decorator';
import {_nbind as _globals} from './Globals';
import {_nbind as _type} from './BindingType';
import {_nbind as _caller} from './Caller';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export var BindType = _type.BindType;
}

export namespace _nbind {

	export var throwError: typeof _globals.throwError;

	export var makeJSCaller: typeof _caller.makeJSCaller;

	export class CallbackType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		makeWireWrite = (expr: string) => '_nbind.registerCallback(' + expr + ')';
	}

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

		var num = callbackFreeList.pop() || callbackList.length;

		callbackList[num] = func;
		callbackRefCountList[num] = 1;

		return(num);
	}

	export function unregisterCallback(num: number) {
		callbackList[num] = null;
		callbackFreeList.push(num);
	}

	@prepareNamespace('_nbind')
	export class _ {}
}

@exportLibrary
class nbind {

	@dep('_nbind')
	static _nbind_register_callback_signature(
		typeListPtr: number,
		typeCount: number
	) {
		var typeList = Array.prototype.slice.call(HEAPU32, typeListPtr / 4, typeListPtr / 4 + typeCount);
		var num = _nbind.callbackSignatureList.length;

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
