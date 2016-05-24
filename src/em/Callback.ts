// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';
import {_nbind as _globals} from './Globals';
import {_nbind as _type} from './BindingType';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export var BindType = _type.BindType;
}

export namespace _nbind {

	export var throwError: typeof _globals.throwError;

	export class CallbackType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		needsWireWrite: boolean = true;

		makeWireWrite(expr: string) {
			return('_nbind.registerCallback(' + expr + ')');
		}
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
