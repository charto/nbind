// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {
	setEvil,
	defineHidden,
	prepareNamespace
} from 'emscripten-library-decorator';

import { _nbind as _globals } from './Globals';
import { _nbind as _class } from './BindClass';
import { _nbind as _gc } from './GC';
import { StateFlags, TypeFlags, PolicyTbl } from '../Type';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

const _defineHidden = defineHidden;

export namespace _nbind {

	export let ptrMarker: typeof _class.ptrMarker;

	export let mark: typeof _gc.mark;

	/** Base class for wrapped instances of bound C++ classes.
	  * Note that some hacks avoid ever constructing this,
	  * so initializing values inside its definition won't work. */

	export class Wrapper {
		persist() { this.__nbindState |= StateFlags.isPersistent; }

		free?(): void;

		/* tslint:disable:variable-name */

		__nbindFlags: TypeFlags;
		__nbindState: StateFlags;

		/** Dynamically set by _nbind_register_constructor.
		  * Calls the C++ constructor and returns a numeric heap pointer. */
		__nbindConstructor: (...args: any[]) => number;
		__nbindValueConstructor: _globals.Func;
		__nbindType: _class.BindClass;

		__nbindPtr: number;
		__nbindShared: number;

		/* tslint:enable:variable-name */
	}

	export function makeBound(
		policyTbl: PolicyTbl,
		bindClass: _class.BindClass
	) {
		class Bound extends Wrapper {
			constructor(marker: {}, flags: number, ptr: number, shared?: number) {
				super();

				if(!(this instanceof Bound)) {

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

			free() {
				bindClass.destroy.call(this, this.__nbindShared, this.__nbindFlags);

				this.__nbindState |= StateFlags.isDeleted;

				disableMember(this, '__nbindShared');
				disableMember(this, '__nbindPtr');
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

	function disableMember(obj: Wrapper, name: string) {
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
