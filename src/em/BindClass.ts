// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import { setEvil, prepareNamespace } from 'emscripten-library-decorator';
import { _nbind as _globals } from './Globals';
import { _nbind as _type } from './BindingType';
import { _nbind as _value } from './ValueObj';
import { _nbind as _caller } from './Caller';
import { _nbind as _wrapper } from './Wrapper';
import { _nbind as _resource } from './Resource';
import { SignatureType } from '../common';
import { TypeFlags, TypeSpecWithName, TypeSpecWithParam, PolicyTbl } from '../Type';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {
	export const BindType = _type.BindType;
	export const Wrapper = _wrapper.Wrapper;
}

export namespace _nbind {

	export let addMethod: typeof _globals.addMethod;
	export let getType: typeof _globals.getType;

	export let pushValue: typeof _value.pushValue;
	export let popValue: typeof _value.popValue;

	export let resources: typeof _resource.resources;

	export let makeCaller: typeof _caller.makeCaller;
	export let makeMethodCaller: typeof _caller.makeMethodCaller;

	type Wrapper = _wrapper.Wrapper;
	export let makeBound: typeof _wrapper.makeBound;

	// Any subtype (not instance but type) of Wrapper.
	// Declared as anything that constructs something compatible with Wrapper.

	interface WrapperClass {
		new(marker: {}, flags: number, ptr: number, shared?: number): Wrapper;

		[key: string]: any;
	}

	export const ptrMarker = {};

	export let callUpcast: (upcast: number, ptr: number) => number;

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

			if(spec.paramList) {
				this.classType = (spec.paramList[0] as BindClass).classType;
				this.proto = this.classType.proto;
			} else this.classType = this;
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
							setter = makeMethodCaller(src.ptrType, spec) as (arg: any) => void;
							break;

						case SignatureType.getter:
							Object.defineProperty(target, spec.name, {
								configurable: true,
								enumerable: false,
								get: makeMethodCaller(src.ptrType, spec) as () => any,
								set: setter
							});
							break;

						case SignatureType.method:
							caller = makeMethodCaller(src.ptrType, spec);
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

			this.registerMethods(src, firstSuper < 0);
		}

		finish() {
			if(this.ready) return(this);
			this.ready = true;

			this.superList = (this.superIdList || []).map(
				(superId: number) => (getType(superId) as BindClass).finish()
			);

			const Bound = this.proto;

			if(this.superList.length) {
				const Proto = function(this: Wrapper) {
					this.constructor = Bound;
				} as any as { new(): Wrapper };

				Proto.prototype = this.superList[0].proto.prototype;
				Bound.prototype = new Proto();
			}

			if(Bound != Module as any) Bound.prototype.__nbindType = this;

			this.registerSuperMethods(this, 1, {});
			return(this);
		}

		upcastStep(dst: BindClass, ptr: number): number {
			if(dst == this) return(ptr);

			for(let i = 0; i < this.superList.length; ++i) {
				const superPtr = this.superList[i].upcastStep(
					dst,
					callUpcast(this.upcastList[i], ptr)
				);
				if(superPtr) return(superPtr);
			}

			return(0);
		}

		wireRead = (arg: number) => popValue(arg, this.ptrType);
		wireWrite = (arg: any) => pushPointer(arg, this.ptrType, true);

		// Optional type conversion code
		// makeWireWrite = (expr: string) => '_nbind.pushValue(' + expr + ')';

		// Reference to JavaScript class for wrapped instances
		// of this C++ class.

		classType: BindClass;
		proto: WrapperClass;
		ptrType: BindClassPtr;

		destroy: (shared: number, flags: number) => void;

		/** Number of super classes left to initialize. */
		pendingSuperCount = 0;

		ready = false;

		superIdList: number[];
		superList: BindClass[];
		upcastList: number[];
		methodTbl: { [name: string]: MethodSpec[] } = {};

		static list: BindClass[] = [];
	}

	export function popPointer(ptr: number, type: BindClassPtr) {
		return(ptr ? new type.proto(ptrMarker, type.flags, ptr) : null);
	}

	export function pushPointer(obj: Wrapper, type: BindClassPtr, tryValue?: boolean) {
		if(!(obj instanceof Wrapper)) {
			if(tryValue) {
				return(pushValue(obj));
			} else throw(new Error('Type mismatch'));
		}

		let ptr = obj.__nbindPtr;
		let objType = (obj.__nbindType).classType;
		let classType = type.classType;

		if(obj instanceof type.proto) {
			// Fast path, requested type is in object's prototype chain.

			while(objType != classType) {
				ptr = callUpcast(objType.upcastList[0], ptr);
				objType = objType.superList[0];
			}
		} else {
			ptr = objType.upcastStep(classType, ptr);
			if(!ptr) throw(new Error('Type mismatch'));
		}

		return(ptr);
	}

	function pushMutablePointer(obj: Wrapper, type: BindClassPtr) {
		const ptr = pushPointer(obj, type);

		if(obj.__nbindFlags & TypeFlags.isConst) {
			throw(new Error('Passing a const value as a non-const argument'));
		}

		return(ptr);
	}

	export class BindClassPtr extends BindType {
		constructor(spec: TypeSpecWithParam) {
			super(spec);

			this.classType = (spec.paramList[0] as BindClass).classType;
			this.proto = this.classType.proto;

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

		classType: BindClass;
		proto: WrapperClass;
	}

	export function popShared(ptr: number, type: SharedClassPtr) {
		const shared = HEAPU32[ptr / 4];
		const unsafe = HEAPU32[ptr / 4 + 1];

		return(unsafe ? new type.proto(ptrMarker, type.flags, unsafe, shared) : null);
	}

	function pushShared(obj: Wrapper, type: SharedClassPtr) {
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

			this.classType = (spec.paramList[0] as BindClass).classType;
			this.proto = this.classType.proto;

			const isConst = spec.flags & TypeFlags.isConst;
			const push = isConst ? pushShared : pushMutableShared;

			this.wireRead = (arg: number) => popShared(arg, this);
			this.wireWrite = (arg: any) => push(arg, this);
		}

		readResources = [ resources.pool ];

		classType: BindClass;
		proto: WrapperClass;
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}
