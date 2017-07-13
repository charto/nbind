// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file contains the type conversion base class and handles conversion of
// C++ primitive types to / from JavaScript. Following emscripten conventions,
// the type passed between the two is called WireType.
// Anything from the standard library is instead in BindingStd.ts

import { setEvil, prepareNamespace } from 'emscripten-library-decorator';
import { _nbind as _globals } from './Globals';
import { _nbind as _resource } from './Resource';
import { typeModule, TypeFlags, TypeSpecWithSize, PolicyTbl } from '../Type';

const _typeModule = typeModule;

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {

	export const { Type, makeType, getComplexType, structureList } = _typeModule(_typeModule);

	export let Pool: typeof _globals.Pool;

	export let resources: typeof _resource.resources;

	// A type definition, which registers itself upon construction.

	export class BindType extends Type {
		needsWireRead(policyTbl: PolicyTbl | null) {
			return(!!this.wireRead || !!this.makeWireRead);
		}

		needsWireWrite(policyTbl: PolicyTbl | null) {
			return(!!this.wireWrite || !!this.makeWireWrite);
		}

		readResources: _resource.Resource[];
		writeResources: _resource.Resource[];

		heap: any = HEAPU32;
		ptrSize = 4;
	}

	export class PrimitiveType extends BindType {
		constructor(spec: TypeSpecWithSize) {
			super(spec);

			const heapTbl: { [bits: number]: any } = (
				spec.flags & TypeFlags.isFloat ? {
					32: HEAPF32,
					64: HEAPF64
				} : spec.flags & TypeFlags.isUnsigned ? {
					8: HEAPU8,
					16: HEAPU16,
					32: HEAPU32
				} : {
					8: HEAP8,
					16: HEAP16,
					32: HEAP32
				}
			);

			this.heap = heapTbl[spec.ptrSize * 8];
			this.ptrSize = spec.ptrSize;
		}

		needsWireWrite(policyTbl: PolicyTbl | null) {
			return(!!policyTbl && !!policyTbl['Strict']);
		}

		makeWireWrite(expr: string, policyTbl: PolicyTbl) {
			return(
				policyTbl && policyTbl['Strict'] && (
				(arg: any) => {
					if(typeof(arg) == 'number') return(arg);
					throw(new Error('Type mismatch'));
				})
			);
		}
	}

	// Push a string to the C++ stack, zero-terminated and UTF-8 encoded.

	export function pushCString(str: string, policyTbl?: PolicyTbl) {
		if(str === null || str === undefined) {
			if(policyTbl && policyTbl['Nullable']) {
				return(0);
			} else throw(new Error('Type mismatch'));
		}

		if(policyTbl && policyTbl['Strict']) {
			if(typeof(str) != 'string') throw(new Error('Type mismatch'));
		} else str = str.toString();

		const length = Module.lengthBytesUTF8(str) + 1;
		const result = Pool.lalloc(length);

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
		makeWireWrite(expr: string, policyTbl: PolicyTbl) {
			return((arg: any) => pushCString(arg, policyTbl));
		}

		wireRead = popCString;
		wireWrite = pushCString;

		readResources = [ resources.pool ];
		writeResources = [ resources.pool ];
	}

	// Booleans are returned as numbers from Asm.js.
	// Prefixing with !! converts them to JavaScript booleans.

	export class BooleanType extends BindType {
		needsWireWrite(policyTbl: PolicyTbl | null) {
			return(!!policyTbl && !!policyTbl['Strict']);
		}

		makeWireRead(expr: string) { return('!!(' + expr + ')'); }

		makeWireWrite(expr: string, policyTbl: PolicyTbl) {
			return(
				policyTbl && policyTbl['Strict'] && (
				(arg: any) => {
					if(typeof(arg) == 'boolean') return(arg);
					throw(new Error('Type mismatch'));
				}) || expr
			);
		}

		wireRead = (arg: number) => !!arg;
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}
