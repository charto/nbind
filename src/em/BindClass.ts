// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {
	setEvil,
	defineHidden,
	prepareNamespace
} from 'emscripten-library-decorator';

import {_nbind as _globals} from './Globals';
import {_nbind as _type} from './BindingType';
import {_nbind as _value} from './ValueObj';
import {_nbind as _resource} from './Resource';
import {_nbind as _gc} from './GC';
import {StateFlags, TypeFlags, TypeSpec, PolicyTbl} from '../Type';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

const _defineHidden = defineHidden;

export namespace _nbind {
	export var BindType = _type.BindType;
}

export namespace _nbind {

	export var pushValue: typeof _value.pushValue;
	export var popValue: typeof _value.popValue;

	export var resources: typeof _resource.resources;

	export var mark: typeof _gc.mark;

	// Base class for wrapped instances of bound C++ classes.

	export class Wrapper {
		persist() { this.__nbindState |= StateFlags.isPersistent; }

		free: () => void;

		/* tslint:disable:variable-name */

		__nbindFlags: TypeFlags;
		__nbindState: StateFlags;

		/** Dynamically set by _nbind_register_constructor.
		  * Calls the C++ constructor and returns a numeric heap pointer. */
		__nbindConstructor: (...args: any[]) => number;
		__nbindValueConstructor: _globals.Func;

		__nbindPtr: number;
		__nbindShared: number;

		/* tslint:enable:variable-name */
	}

	// Any subtype (not instance but type) of Wrapper.
	// Declared as anything that constructs something compatible with Wrapper.

	interface WrapperClass {
		new(marker: {}, flags: number, ptr: number, shared?: number): Wrapper;
	}

	const ptrMarker = {};

	export function makeBound(policyTbl: PolicyTbl) {
		class Bound extends Wrapper {
			constructor(marker: {}, flags: number, ptr: number, shared?: number) {
				// super() never gets called here but TypeScript 1.8 requires it.
				if((false && super()) || !(this instanceof Bound)) {

					// Constructor called without new operator.
					// Make correct call with given arguments.
					// Few ways to do this work. This one should. See:
					// http://stackoverflow.com/questions/1606797
					// /use-of-apply-with-new-operator-is-this-possible

					return(new (Function.prototype.bind.apply(
						Bound, // arguments.callee
						Array.prototype.concat.apply([null], arguments)
					))());
				}

				super();

				let nbindFlags = flags;
				let nbindPtr = ptr;
				let nbindShared = shared;

				if(marker !== ptrMarker) {
					let wirePtr = this.__nbindConstructor.apply(this, arguments);

					nbindFlags = TypeFlags.isSharedClassPtr | TypeFlags.isSharedPtr;
					nbindShared = HEAPU32[wirePtr / 4];
					nbindPtr = HEAPU32[wirePtr / 4 + 1];
				}

				const spec = {
					configurable: true,
					enumerable: false,
					value: null as any,
					writable: false
				};

				const propTbl: { [key: string]: any } = {
					'__nbindFlags': nbindFlags,
					'__nbindPtr': nbindPtr
				};

				if(nbindShared) {
					propTbl['__nbindShared'] = nbindShared;
					mark(this);
				}

				for(let key of Object.keys(propTbl)) {
					spec.value = propTbl[key];
					Object.defineProperty(this, key, spec);
				}

				_defineHidden(StateFlags.none)(this, '__nbindState');
			}

			@_defineHidden()
			// tslint:disable-next-line:variable-name
			__nbindConstructor: (...args: any[]) => number;

			@_defineHidden()
			// tslint:disable-next-line:variable-name
			__nbindValueConstructor: _globals.Func;

			@_defineHidden(policyTbl)
			// tslint:disable-next-line:variable-name
			__nbindPolicies: { [name: string]: boolean };
		}

		return(Bound);
	}

	// Base class for all bound C++ classes (not their instances),
	// also inheriting from a generic type definition.

	export class BindClass extends BindType {
		constructor(spec: TypeSpec, proto?: WrapperClass) {
			super(spec);

			this.proto = proto || (spec.paramList[0] as BindClass).proto;
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

	export function popPointer(ptr: number, type: BindClassPtr) {
		return(ptr ? new type.proto(ptrMarker, type.flags, ptr) : null);
	}

	function pushPointer(obj: Wrapper, type: BindClassPtr) {
		if(!(obj instanceof type.proto)) throw(new Error('Type mismatch'));
		return(obj.__nbindPtr);
	}

	function pushMutablePointer(obj: Wrapper, type: BindClassPtr) {
		if(!(obj instanceof type.proto)) throw(new Error('Type mismatch'));

		if(obj.__nbindFlags & TypeFlags.isConst) {
			throw(new Error('Passing a const value as a non-const argument'));
		}

		return(obj.__nbindPtr);
	}

	export class BindClassPtr extends BindType {
		constructor(spec: TypeSpec) {
			super(spec);

			this.proto = (spec.paramList[0] as BindClass).proto;

			const isConst = spec.flags & TypeFlags.isConst;
			const isValue = (
				(this.flags & TypeFlags.refMask) == TypeFlags.isReference &&
				(spec.flags & TypeFlags.isValueObject)
			);

			const push = isConst ? pushPointer : pushMutablePointer;
			const pop = isValue ? popValue : popPointer;

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
	}

	export function popShared(ptr: number, type: BindClassPtr) {
		const shared = HEAPU32[ptr / 4];
		const unsafe = HEAPU32[ptr / 4 + 1];

		return(unsafe ? new type.proto(ptrMarker, type.flags, unsafe, shared) : null);
	}

	function pushShared(obj: Wrapper, type: BindClassPtr) {
		if(!(obj instanceof type.proto)) throw(new Error('Type mismatch'));

		return(obj.__nbindShared);
	}

	function pushMutableShared(obj: Wrapper, type: BindClassPtr) {
		if(!(obj instanceof type.proto)) throw(new Error('Type mismatch'));

		if(obj.__nbindFlags & TypeFlags.isConst) {
			throw(new Error('Passing a const value as a non-const argument'));
		}

		return(obj.__nbindShared);
	}

	export class SharedClassPtr extends BindType {
		constructor(spec: TypeSpec) {
			super(spec);

			this.proto = (spec.paramList[0] as BindClass).proto;

			const isConst = spec.flags & TypeFlags.isConst;
			const push = isConst ? pushShared : pushMutableShared;

			this.wireRead = (arg: number) => popShared(arg, this);
			this.wireWrite = (arg: any) => push(arg, this);
		}

		readResources = [ resources.pool ];

		proto: WrapperClass;
	}

	export function disableMember(obj: Wrapper, name: string) {
		function die() { throw(new Error('Accessing deleted object')); }

		Object.defineProperty(obj, name, {
			configurable: false,
			enumerable: false,
			get: die,
			set: die
		});
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}
