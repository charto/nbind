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

	export type PolicyTbl = _globals.PolicyTbl;

	export var pushValue: typeof _value.pushValue;
	export var popValue: typeof _value.popValue;

	// Base class for wrapped instances of bound C++ classes.

	export class Wrapper {
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
	}

	// Any subtype (not instance but type) of Wrapper.
	// Declared as anything that constructs something compatible with Wrapper.

	interface WrapperClass {
		new(...args: any[]): Wrapper;
	}

	// Base class for all bound C++ classes (not their instances),
	// also inheriting from a generic type definition.

	export class BindClass extends BindType {
		constructor(id: number, name: string, proto: WrapperClass) {
			super(id, name);

			this.proto = proto;
		}

		wireRead = popValue;
		wireWrite = pushValue;

		// Optional type conversion code
		// makeWireWrite = (expr: string) => '_nbind.pushValue(' + expr + ')';

		// Reference to JavaScript class for wrapped instances
		// of this C++ class.

		proto: WrapperClass;
	}

	export var ptrMarker = {};

	export function popPointer(ptr: number, type: BindClassPtr) {
		if(ptr === 0) return(null);

		const obj = new type.proto(ptrMarker, ptr, type.flags);

		return(obj);
	}

	export function pushPointer(obj: any, type: BindClassPtr, policyTbl?: PolicyTbl) {
		// Handle null pointers.
		if(!obj && policyTbl['Nullable']) return(0);
		if(!(obj instanceof type.proto)) throw(new Error('Type mismatch'));

		if((obj.__nbindFlags & Wrapper.constant) && !(type.flags & Wrapper.constant)) {
			throw(new Error('Passing a const value as a non-const argument'));
		}

		return(obj.__nbindPtr);
	}

	export class BindClassPtr extends BindType {
		constructor(id: number, name: string, proto: WrapperClass, flags?: number) {
			super(id, name);

			this.proto = proto;
			this.flags = flags || 0;
		}

		makeWireWrite = (expr: string, policyTbl: PolicyTbl) => (
			(arg: any) => pushPointer(arg, this, policyTbl)
		);
		wireRead = (arg: number) => popPointer(arg, this);
		wireWrite = (arg: any) => pushPointer(arg, this);

		proto: WrapperClass;
		flags: number;
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}
