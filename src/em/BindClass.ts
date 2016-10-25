// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';
import {_nbind as _globals} from './Globals';
import {_nbind as _type} from './BindingType';
import {_nbind as _value} from './ValueObj';
import {_nbind as _caller} from './Caller';
import {_nbind as _wrapper} from './Wrapper';
import {_nbind as _resource} from './Resource';
import {SignatureType} from '../common';
import {TypeFlags, TypeSpecWithName, TypeSpecWithParam, PolicyTbl} from '../Type';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export var BindType = _type.BindType;
}

export namespace _nbind {

	export var addMethod: typeof _globals.addMethod;
	export var getType: typeof _globals.getType;

	export var pushValue: typeof _value.pushValue;
	export var popValue: typeof _value.popValue;

	export var resources: typeof _resource.resources;

	export var makeCaller: typeof _caller.makeCaller;
	export var makeMethodCaller: typeof _caller.makeMethodCaller;

	type Wrapper = _wrapper.Wrapper;
	export var makeBound: typeof _wrapper.makeBound;

	// Any subtype (not instance but type) of Wrapper.
	// Declared as anything that constructs something compatible with Wrapper.

	interface WrapperClass {
		new(marker: {}, flags: number, ptr: number, shared?: number): Wrapper;

		[key: string]: any;
	}

	export const ptrMarker = {};

	export interface MethodSpec {
		name: string;
		title: string;
		signatureType?: SignatureType;
		boundID?: number;
		policyTbl?: PolicyTbl;
		typeList?: (number | string)[];
		ptr: number;
		direct?: number;
		num?: number;
		flags?: TypeFlags;
	}

	// Base class for all bound C++ classes (not their instances),
	// also inheriting from a generic type definition.

	export class BindClass extends BindType {
		constructor(spec: TypeSpecWithName) {
			super(spec);

			if(spec.paramList) this.proto = (spec.paramList[0] as BindClass).proto;
		}

		makeBound(policyTbl: PolicyTbl) {
			const Bound = makeBound(policyTbl, this);

			this.proto = Bound;
			this.ptrType.proto = Bound;

			return(Bound);
		}

		addMethod(spec: MethodSpec) {
			const overloadList = this.methodTbl[spec.name] || [];

			overloadList.push(spec);

			this.methodTbl[spec.name] = overloadList;
		}

		registerMethods(src: BindClass, staticOnly: boolean) {
			let setter: ((arg: any) => void) | undefined;

			for(let name of Object.keys(src.methodTbl)) {
				const overloadList = src.methodTbl[name];

				for(let spec of overloadList) {
					let target: any;
					let caller: any;

					target = this.proto.prototype;

					if(staticOnly && spec.signatureType != SignatureType.func) continue;

					switch(spec.signatureType) {
						case SignatureType.func:
							target = this.proto;

						// tslint:disable-next-line:no-switch-case-fall-through
						case SignatureType.construct:
							caller = makeCaller(spec);
							addMethod(target, spec.name, caller, spec.typeList!.length - 1);
							break;

						case SignatureType.setter:
							setter = makeMethodCaller(spec) as (arg: any) => void;
							break;

						case SignatureType.getter:
							Object.defineProperty(target, spec.name, {
								configurable: true,
								enumerable: false,
								get: makeMethodCaller(spec) as () => any,
								set: setter
							});
							break;

						case SignatureType.method:
							caller = makeMethodCaller(spec);
							addMethod(target, spec.name, caller, spec.typeList!.length - 1);
							break;

						default:
							break;
					}
				}
			}
		}

		registerSuperMethods(
			src: BindClass,
			firstSuper: number,
			visitTbl: { [name: string]: boolean }
		) {
			if(visitTbl[src.name]) return;
			visitTbl[src.name] = true;

			this.registerMethods(src, firstSuper < 0);

			let superNum = 0;
			let nextFirst: number;

			for(let superId of src.superIdList || []) {
				const superClass = getType(superId) as BindClass;

				if(superNum++ < firstSuper || firstSuper < 0) {
					nextFirst = -1;
				} else {
					nextFirst = 0;
				}

				this.registerSuperMethods(superClass, nextFirst, visitTbl);
			}
		}

		finish() {
			if(this.ready) return;
			this.ready = true;

			let superClass: BindClass | undefined;
			let firstSuper: BindClass | undefined;

			for(let superId of this.superIdList || []) {
				superClass = getType(superId) as BindClass;
				superClass.finish();

				firstSuper = firstSuper || superClass;
			}

			if(firstSuper) {
				const Bound = this.proto;
				const Proto = function(this: Wrapper) {
					this.constructor = Bound;
				} as any as { new(): Wrapper };

				Proto.prototype = firstSuper.proto.prototype;
				Bound.prototype = new Proto();
			}

			this.registerSuperMethods(this, 1, {});
		}

		wireRead = (arg: number) => popValue(arg, this.ptrType);
		wireWrite = pushValue;

		// Optional type conversion code
		// makeWireWrite = (expr: string) => '_nbind.pushValue(' + expr + ')';

		// Reference to JavaScript class for wrapped instances
		// of this C++ class.

		proto: WrapperClass;
		ptrType: BindClassPtr;

		destroy: (shared: number, flags: number) => void;

		/** Number of super classes left to initialize. */
		pendingSuperCount = 0;

		ready = false;

		superIdList: number[];
		methodTbl: { [name: string]: MethodSpec[] } = {};

		static list: BindClass[] = [];
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
		constructor(spec: TypeSpecWithParam) {
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
		constructor(spec: TypeSpecWithParam) {
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

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}
