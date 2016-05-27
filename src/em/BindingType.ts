// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file contains the type conversion base class and handles conversion of
// C++ primitive types to / from JavaScript. Following emscripten conventions,
// the type passed between the two is called WireType.
// Anything from the standard library is instead in BindingStd.ts

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';
import {_nbind as _globals} from './Globals';
import {_nbind as _resource} from './Resource';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export var Pool = _globals.Pool;
}

export namespace _nbind {

	export var typeTbl: typeof _globals.typeTbl;
	export var typeList: typeof _globals.typeList;

	export var resources: typeof _resource.resources;

	// A type definition, which registers itself upon construction.

	export class BindType {
		constructor(id: number, name: string) {
			this.id = id;
			this.name = name;

			typeTbl[name] = this;
			typeList[id] = this;
		}

		wireRead: (arg: number) => any;
		wireWrite: (arg: any) => number;

		makeWireRead: (expr: string, convertParamList?: any[], num?: number) => string;
		makeWireWrite: (expr: string, convertParamList?: any[], num?: number) => string;

		readResources: _resource.Resource[];
		writeResources: _resource.Resource[];

		id: number;
		name: string;

		heap: any = HEAPU32;
		ptrSize = 4;
	}

	export class PrimitiveType extends BindType {
		constructor(id: number, name: string, size: number, isUnsigned: boolean, isFloat: boolean) {
			super(id, name);

			var heapTbl: { [bits: number]: any } = (
				isFloat ? {
					32: HEAPF32,
					64: HEAPF64
				} : isUnsigned ? {
					8: HEAPU8,
					16: HEAPU16,
					32: HEAPU32,
				} : {
					8: HEAP8,
					16: HEAP16,
					32: HEAP32,
				}
			);

			this.heap = heapTbl[size * 8];
			this.ptrSize = size;
		}
	}

	// Push a string to the C++ stack, zero-terminated and UTF-8 encoded.

	export function pushCString(str: string) {
		if(str === null || str === undefined) return(0);
		str = str.toString();

		var length = Module.lengthBytesUTF8(str) + 1;
		var result = Pool.lalloc(length);

		// Convert the string and append a zero byte.
		Module.stringToUTF8Array(str, HEAPU8, result, length);

		return(result);
	}

	// Read a zero-terminated, UTF-8 encoded string from the C++ stack.

	export function popCString(ptr: number) {
		if(ptr === 0) return(null);

		return(Module.Pointer_stringify(ptr));
	}

	// Zero-terminated 'const char *' style string, passed through the C++ stack.

	export class CStringType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		wireRead = popCString;
		wireWrite = pushCString;

		// Optional type conversion code
		// makeWireRead = (expr: string) => '_nbind.popCString(' + expr + ')';
		// makeWireWrite = (expr: string) => '_nbind.pushCString(' + expr + ')';

		readResources = [ resources.pool ];
		writeResources = [ resources.pool ];
	}

	// Booleans are returned as numbers from Asm.js.
	// Prefixing with !! converts them to JavaScript booleans.

	export class BooleanType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		wireRead = (arg: number) => !!arg;

		makeWireRead = (expr: string) => '!!(' + expr + ')';
	}

	@prepareNamespace('_nbind')
	export class _ {}
}
