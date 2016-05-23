// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';
import {_nbind as _main} from './nbind-em';
import {_nbind as _resource} from './Resource';

setEvil((code: string) => eval(code));

export namespace _nbind {

	export var typeTbl: typeof _main.typeTbl;
	export var typeList: typeof _main.typeList;

	export var resources: typeof _resource.resources;

	// A type definition, which registers itself upon construction.

	export class BindType {
		constructor(id: number, name: string) {
			this.id = id;
			this.name = name;

			// Namespace name is needed here, or TypeScript will mangle it.
			_nbind.typeTbl[name] = this;
			_nbind.typeList[id] = this;
		}

		// TODO: maybe this should be an abstract base class and these versions of wire type conversions
		// should be in a subclass called PrimitiveType.

		needsWireRead: boolean = false;

		makeWireRead(expr: string) {
			return(expr);
		}

		needsWireWrite: boolean = false;

		makeWireWrite(expr: string) {
			return(expr);
		}

		needsResources: _resource.Resource[] = null;

		id: number;
		name: string;
	}

	// Push a string to the C++ stack, zero-terminated and UTF-8 encoded.

	export function pushCString(str: string) {
		if(str === null || str === undefined) return(0);
		str = str.toString();

		var length = Module.lengthBytesUTF8(str) + 1;
		var result = Runtime.stackAlloc(length);

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

		needsWireRead: boolean = true;

		makeWireRead(expr: string) {
			return('_nbind.popCString(' + expr + ')');
		}

		needsWireWrite: boolean = true;

		makeWireWrite(expr: string) {
			return('_nbind.pushCString(' + expr + ')');
		}

		needsResources = [ resources.stack ];
	}

	// Booleans are returned as numbers from Asm.js.
	// Prefixing with !! converts them to JavaScript booleans.

	export class BooleanType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		needsWireRead: boolean = true;

		makeWireRead(expr: string) {
			return('!!(' + expr + ')');
		}
	}

	export class CallbackType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		needsWireWrite: boolean = true;

		makeWireWrite(expr: string) {
			return('_nbind.registerCallback(' + expr + ')');
		}
	}

	export function pushString(str: string) {
		if(str === null || str === undefined) return(0);
		str = str.toString();

		var length = Module.lengthBytesUTF8(str);

		// 32-bit length, string and a zero terminator
		// (stringToUTF8Array insists on adding it)

		var result = Runtime.stackAlloc(4 + length + 1);

		HEAPU32[result / 4] = length;
		Module.stringToUTF8Array(str, HEAPU8, result + 4, length + 1);

		return(result);
	}

	export function popString(ptr: number) {
		if(ptr === 0) return(null);

		var length = HEAPU32[ptr / 4];

		return(Module.Pointer_stringify(ptr + 4, length));
	}

	export class StringType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		needsWireRead: boolean = true;

		makeWireRead(expr: string) {
			return('_nbind.popString(' + expr + ')');
		}

		needsWireWrite: boolean = true;

		makeWireWrite(expr: string) {
			return('_nbind.pushString(' + expr + ')');
		}

		needsResources = [ resources.stack ];
	}

	// Special type that constructs a new object.

	export class CreateValueType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		needsWireWrite: boolean = true;

		makeWireWrite(expr: string) {
			return('((_nbind.value=new ' + expr + '),0)');
		}
	}

	// Base class for all bound C++ classes (not their instances),
	// also inheriting from a generic type definition.

	export class BindClass extends BindType {
		constructor(id: number, name: string, proto: _main.WrapperClass) {
			super(id, name);

			this.proto = proto;
		}

		// Reference to JavaScript class for wrapped instances
		// of this C++ class.

		proto: _main.WrapperClass;

		needsWireRead: boolean = true;

		makeWireRead(expr: string) {
			return('(' +
				expr + '||' +
				'_nbind.throwError("Value type JavaScript class is missing or not registered"),' +
				'_nbind.value' +
			')');
		}

		needsWireWrite: boolean = true;

		makeWireWrite(expr: string) {
			// TODO: free the list item allocated here.

			return('_nbind.storeValue(' + expr + ')');
		}
	}

	@prepareNamespace('_nbind')
	export class _ {}
}
