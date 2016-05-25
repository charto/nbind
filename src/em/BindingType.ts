// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';
import {_nbind as _globals} from './Globals';
import {_nbind as _resource} from './Resource';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

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

		// TODO: maybe this should be an abstract base class and these versions of wire type conversions
		// should be in a subclass called PrimitiveType.

		makeWireRead: (expr: string, convertParamList?: any[], num?: number) => string;
		makeWireWrite: (expr: string, convertParamList?: any[], num?: number) => string;

		readResources: _resource.Resource[];
		writeResources: _resource.Resource[];

		id: number;
		name: string;
	}

	// Push a string to the C++ stack, zero-terminated and UTF-8 encoded.

	export function pushCString(str: string) {
		if(str === null || str === undefined) return(0);
		str = str.toString();

		var length = Module.lengthBytesUTF8(str) + 1;
		var result = Runtime.stackAlloc(length);

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

		makeWireRead = (expr: string) => '_nbind.popCString(' + expr + ')';
		makeWireWrite = (expr: string) => '_nbind.pushCString(' + expr + ')';

		readResources = [ resources.pool ];
		writeResources = [ resources.stack ];
	}

	// Booleans are returned as numbers from Asm.js.
	// Prefixing with !! converts them to JavaScript booleans.

	export class BooleanType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		makeWireRead = (expr: string) => '!!(' + expr + ')';
	}

	@prepareNamespace('_nbind')
	export class _ {}
}
