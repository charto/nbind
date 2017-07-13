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

import { _nbind as _globals } from './Globals';
import { _nbind as _type } from './BindingType';
import { _nbind as _caller } from './Caller';
import { _nbind as _external } from './External';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export const BindType = _type.BindType;
	export const External = _external.External;
}

export namespace _nbind {

	export let readTypeIdList: typeof _globals.readTypeIdList;
	export let throwError: typeof _globals.throwError;

	export let makeJSCaller: typeof _caller.makeJSCaller;

	// List of invoker functions for all argument and return value combinations
	// seen so far.

	export let callbackSignatureList: _globals.Func[] = [];

	export class CallbackType extends BindType {
		wireWrite = (func: _globals.Func) => {
			if(typeof(func) != 'function') _nbind.throwError('Type mismatch');

			return(new External(func).register());
		}

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

}
