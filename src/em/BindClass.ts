// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';
import {_nbind as _globals} from './Globals';
import {_nbind as _type} from './BindingType';
import {_nbind as _value} from './ValueObj';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export var BindType = _type.BindType;
}

export namespace _nbind {

	type PolicyTbl = _globals.PolicyTbl;

	export var pushValue: typeof _value.pushValue;
	export var popValue: typeof _value.popValue;

	// Base class for wrapped instances of bound C++ classes.

	export class Wrapper {
		persist() { this.__nbindFlags |= Wrapper.persistent; }

		free: () => void;

		/* tslint:disable:variable-name */

		__nbindFlags: number;

		/** Dynamically set by _nbind_register_constructor.
		  * Calls the C++ constructor and returns a numeric heap pointer. */
		__nbindConstructor: (...args: any[]) => number;
		__nbindValueConstructor: _globals.Func;

		/** __nbindConstructor return value. */
		__nbindPtr: number;

		/* tslint:enable:variable-name */

		static constant = 1;
		static reference = 2;
		static persistent = 4;
		static originJS = 8;
	}

	// Any subtype (not instance but type) of Wrapper.
	// Declared as anything that constructs something compatible with Wrapper.

	interface WrapperClass {
		new(...args: any[]): Wrapper;
	}

	// Base class for all bound C++ classes (not their instances),
	// also inheriting from a generic type definition.

	export class BindClass extends BindType {
		constructor(id: number, name: string, proto: WrapperClass, ptrType: BindClassPtr) {
			super(id, name);

			this.proto = proto;
			this.ptrType = ptrType;
		}

		wireRead = (arg: number) => popValue(arg, this.ptrType);
		wireWrite = pushValue;

		// Optional type conversion code
		// makeWireWrite = (expr: string) => '_nbind.pushValue(' + expr + ')';

		// Reference to JavaScript class for wrapped instances
		// of this C++ class.

		proto: WrapperClass;
		ptrType: BindClassPtr;
	}

	export var ptrMarker = {};

	export function popPointer(ptr: number, type: BindClassPtr) {
		if(ptr === 0) return(null);

		const obj = new type.proto(ptrMarker, ptr, type.flags);

		return(obj);
	}

	export function pushPointer(obj: any, type: BindClassPtr) {
		if(!(obj instanceof type.proto)) throw(new Error('Type mismatch'));
		return(obj.__nbindPtr);
	}

	export function pushNonConstPointer(obj: any, type: BindClassPtr) {
		if(!(obj instanceof type.proto)) throw(new Error('Type mismatch'));

		if(obj.__nbindFlags & Wrapper.constant) {
			throw(new Error('Passing a const value as a non-const argument'));
		}

		return(obj.__nbindPtr);
	}

	export class BindClassPtr extends BindType {
		constructor(id: number, name: string, proto: WrapperClass, flags?: number) {
			super(id, name);

			this.proto = proto;
			this.flags = flags || 0;

			const push = flags & Wrapper.constant ? pushPointer : pushNonConstPointer;
			const pop = flags & Wrapper.reference ? popValue : popPointer;

			this.makeWireWrite = (expr: string, policyTbl: PolicyTbl) => (
				policyTbl['Nullable'] ?
					// Handle null pointers.
					(arg: any) => (arg ? push(arg, this) : 0) :
					(arg: any) => push(arg, this)
			);

			this.wireRead = (arg: number) => pop(arg, this);
			this.wireWrite = (arg: any) => push(arg, this);
		}

		proto: WrapperClass;
		flags: number;
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}
