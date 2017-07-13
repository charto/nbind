// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles creating invoker functions for Emscripten dyncalls
// wrapped in type conversions for arguments and return values.

import { setEvil, prepareNamespace } from 'emscripten-library-decorator';
import { _nbind as _globals } from './Globals';
import { _nbind as _type } from './BindingType';
import { _nbind as _class } from './BindClass';
import { _nbind as _wrapper } from './Wrapper';
import { _nbind as _external } from './External';
import { _nbind as _resource } from './Resource';
import { TypeFlags, PolicyTbl } from '../Type';

// Let decorators run eval in current scope to read function source code.
setEvil((code: string) => eval(code));

export namespace _nbind {

	type BindType = _type.BindType;

	type Wrapper = _wrapper.Wrapper;

	type Func = _globals.Func;
	type FuncList = _globals.FuncList;
	type TypeIdList = _globals.TypeIdList;

	export let getTypes: typeof _globals.getTypes;
	export let getDynCall: typeof _globals.getDynCall;

	export let pushPointer: typeof _class.pushPointer;

	export let externalList: _external.External<any>[];

	export let listResources: typeof _resource.listResources;
	export let resources: typeof _resource.resources;

	/** Make a list of argument names a1, a2, a3...
	  * for dynamically generating function source code. */

	function makeArgList(argCount: number) {
		return(
			Array.apply(null, Array(argCount)).map(
				(dummy: any, num: number) => ('a' + (num + 1))
			)
		);
	}

	/** Check if any type on the list requires conversion writing to C++.
	  * Mainly numbers can be passed as-is between Asm.js and JavaScript. */

	function anyNeedsWireWrite(typeList: BindType[], policyTbl: PolicyTbl | null) {
		return(typeList.reduce(
			(result: boolean, type: BindType) =>
				(result || type.needsWireWrite(policyTbl)),
			false
		));
	}

	/** Check if any type on the list requires conversion reading from C++.
	  * Mainly numbers can be passed as-is between Asm.js and JavaScript. */

	function anyNeedsWireRead(typeList: BindType[], policyTbl: PolicyTbl | null) {
		return(typeList.reduce(
			(result: boolean, type: BindType) =>
				(result || !!type.needsWireRead(policyTbl)),
			false
		));
	}

	function makeWireRead(
		convertParamList: any[],
		policyTbl: PolicyTbl | null,
		type: BindType,
		expr: string
	) {
		/** Next free slot number in type converter data list. */
		const paramNum = convertParamList.length;

		if(type.makeWireRead) {
			return(type.makeWireRead(expr, convertParamList, paramNum));
		} else if(type.wireRead) {
			convertParamList[paramNum] = type.wireRead;
			return('(convertParamList[' + paramNum + '](' + expr + '))');
		} else return(expr);
	}

	function makeWireWrite(
		convertParamList: any[],
		policyTbl: PolicyTbl | null,
		type: BindType,
		expr: string
	) {
		let wireWrite: any;
		/** Next free slot number in type converter data list. */
		const paramNum = convertParamList.length;

		if(type.makeWireWrite) {
			wireWrite = type.makeWireWrite(expr, policyTbl, convertParamList, paramNum);
		} else wireWrite = type.wireWrite;

		if(wireWrite) {
			if(typeof(wireWrite) == 'string') {
				return(wireWrite);
			} else {
				convertParamList[paramNum] = wireWrite;
				return('(convertParamList[' + paramNum + '](' + expr + '))');
			}
		} else return(expr);
	}

	/** Dynamically build a function that calls an Asm.js invoker
	  * with appropriate type conversion for complicated types:
		* - Push arguments to stack.
		* - Read return value.
		* - Restore stack pointer if necessary. */

	function buildCallerFunction(
		dynCall: Func,
		ptrType: _class.BindClassPtr | null,
		ptr: number,
		num: number,
		policyTbl: PolicyTbl | null,
		needsWireWrite: boolean,
		prefix: string,
		returnType: BindType,
		argTypeList: BindType[],
		mask?: number,
		err?: () => void
	) {
		const argList = makeArgList(argTypeList.length);
		/** List of arbitrary data for type converters.
		  * Each one may read and write its own slot. */
		const convertParamList: any[] = [];

		// Build code for function call and type conversion.

		const callExpression = makeWireRead(
			convertParamList,
			policyTbl,
			returnType,
			'dynCall(' +
				[prefix].concat(argList.map(
					// TODO: if one wireWrite throws,
					// resources allocated by others may leak!
					(name: string, index: number) => makeWireWrite(
						convertParamList,
						policyTbl,
						argTypeList[index],
						name
					)
				)).join(',') +
			')'
		);

		// Build code to allocate and free the stack etc. if necessary.

		const resourceSet = listResources([returnType], argTypeList);

		const sourceCode = (
			'function(' + argList.join(',') + '){' +
				(mask ? 'this.__nbindFlags&mask&&err();' : '') +
				resourceSet.makeOpen() +
				'var r=' + callExpression + ';' +
				resourceSet.makeClose() +
				'return r;' +
			'}'
		);

		// Use eval to allow JIT compiling the function.

		return(eval('(' + sourceCode + ')') as (...args: any[]) => any);
	}

	/** Dynamically build a function that calls a JavaScript callback invoker
	  * with appropriate type conversion for complicated types:
		* - Read arguments from stack.
		* - Push return value.
		* - Restore stack pointer if necessary. */

	export function buildJSCallerFunction(
		returnType: BindType,
		argTypeList: BindType[]
	) {
		const argList = makeArgList(argTypeList.length);
		/** List of arbitrary data for type converters.
		  * Each one may read and write its own slot. */
		const convertParamList: any[] = [];

		const callExpression = makeWireWrite(
			convertParamList,
			null,
			returnType,
			'_nbind.externalList[num].data(' +
				argList.map(
					// TODO: if one wireRead throws,
					// resources held by others may leak!
					(name: string, index: number) => makeWireRead(
						convertParamList,
						null,
						argTypeList[index],
						name
					)
				).join(',') +
			')'
		);

		const resourceSet = listResources(argTypeList, [returnType]);

		// Let the calling C++ side handle resetting the pool (using the
		// PoolRestore class) after parsing the callback return value passed
		// through the pool.

		resourceSet.remove(_nbind.resources.pool);

		const sourceCode = (
			'function(' + ['dummy', 'num'].concat(argList).join(',') + '){' +
				resourceSet.makeOpen() +
				'var r=' + callExpression + ';' +
				resourceSet.makeClose() +
				'return r;' +
			'}'
		);

		// Use eval to allow JIT compiling the function.

		return(eval('(' + sourceCode + ')') as (...args: any[]) => any);
	}

	/* tslint:disable:indent */

	/** Dynamically create an invoker for a JavaScript callback. */

	export function makeJSCaller(idList: TypeIdList) {
		const argCount = idList.length - 1;

		const typeList = getTypes(idList, 'callback');
		const returnType = typeList[0];
		const argTypeList = typeList.slice(1);
		const needsWireRead = anyNeedsWireRead(argTypeList, null);
		const needsWireWrite = returnType.needsWireWrite(null);

		if(!needsWireWrite && !needsWireRead) {
			switch(argCount) {
				case 0: return(function(dummy: number, num: number) {
				                    return(externalList[num].data(    )); });
				case 1: return(function(dummy: number, num: number, a1: any) {
				                    return(externalList[num].data(       a1    )); });
				case 2: return(function(dummy: number, num: number, a1: any, a2: any) {
				                    return(externalList[num].data(       a1,      a2    )); });
				case 3: return(function(dummy: number, num: number, a1: any, a2: any, a3: any) {
				                    return(externalList[num].data(       a1,      a2,      a3    )); });
				default:
					// Function takes over 3 arguments.
					// Let's create the invoker dynamically then.
					break;
			}
		}

		return(buildJSCallerFunction(
			returnType,
			argTypeList
		));
	}

	/** Dynamically create an invoker function for calling a C++ class method. */

	export function makeMethodCaller(ptrType: _class.BindClassPtr, spec: _class.MethodSpec) {
		const argCount = spec.typeList!.length - 1;

		// The method invoker function adds two arguments to those of the method:
		// - Number of the method in a list of methods with identical signatures.
		// - Target object

		const typeIdList = spec.typeList!.slice(0);
		typeIdList.splice(1, 0, 'uint32_t', spec.boundID!);

		const typeList = getTypes(typeIdList, spec.title);
		const returnType = typeList[0];
		const argTypeList = typeList.slice(3);
		const needsWireRead = returnType.needsWireRead(spec.policyTbl!);
		const needsWireWrite = anyNeedsWireWrite(argTypeList, spec.policyTbl!);
		const ptr = spec.ptr;
		const num = spec.num!;

		const dynCall = getDynCall(typeList, spec.title);

		const mask = ~spec.flags! & TypeFlags.isConst;

		function err() {
			throw(new Error('Calling a non-const method on a const object'));
		}

		if(!needsWireRead && !needsWireWrite) {
			// If there are only a few arguments not requiring type conversion,
			// build a simple invoker function without using eval.

			switch(argCount) {
				case 0: return(function(this: Wrapper) {
					return(this.__nbindFlags & mask ? err() :
				        dynCall(ptr, num, pushPointer(this, ptrType))); });
				case 1: return(function(this: Wrapper,     a1: any) {
					return(this.__nbindFlags & mask ? err() :
				        dynCall(ptr, num, pushPointer(this, ptrType), a1    )); });
				case 2: return(function(this: Wrapper,     a1: any, a2: any) {
					return(this.__nbindFlags & mask ? err() :
				        dynCall(ptr, num, pushPointer(this, ptrType), a1,      a2    )); });
				case 3: return(function(this: Wrapper,     a1: any, a2: any, a3: any) {
					return(this.__nbindFlags & mask ? err() :
				        dynCall(ptr, num, pushPointer(this, ptrType), a1,      a2,      a3    )); });
				default:
					// Function takes over 3 arguments or needs type conversion.
					// Let's create the invoker dynamically then.
					break;
			}
		}

		return(buildCallerFunction(
			dynCall,
			ptrType,
			ptr,
			num,
			spec.policyTbl!,
			needsWireWrite,
			'ptr,num,pushPointer(this,ptrType)',
			returnType,
			argTypeList,
			mask,
			err
		));
	}

	/** Dynamically create an invoker function for calling a C++ function. */

	export function makeCaller(spec: _class.MethodSpec) {
		const argCount = spec.typeList!.length - 1;

		let typeList = getTypes(spec.typeList!, spec.title);
		const returnType = typeList[0];
		const argTypeList = typeList.slice(1);
		const needsWireRead = returnType.needsWireRead(spec.policyTbl!);
		const needsWireWrite = anyNeedsWireWrite(argTypeList, spec.policyTbl!);
		const direct = spec.direct!;
		let dynCall: (...args: any[]) => any;
		let ptr = spec.ptr;

		if(spec.direct && !needsWireRead && !needsWireWrite) {
			// If there are only a few arguments not requiring type conversion,
			// build a simple invoker function without using eval.

			dynCall = getDynCall(typeList, spec.title);

			switch(argCount) {
				case 0: return(() =>
				        dynCall(direct));
				case 1: return((        a1: any) =>
				        dynCall(direct, a1       ));
				case 2: return((        a1: any, a2: any) =>
				        dynCall(direct, a1,      a2       ));
				case 3: return((        a1: any, a2: any, a3: any) =>
				        dynCall(direct, a1,      a2,      a3       ));
				default:
					// Function takes over 3 arguments.
					// Let's create the invoker dynamically then.
					break;
			}

			// Input and output types don't need conversion so omit dispatcher.
			ptr = 0;
		}

		let prefix: string;

		if(ptr) {
			// The function invoker adds an argument to those of the function:
			// - Number of the function in a list of functions with identical signatures.

			const typeIdList = spec.typeList!.slice(0);
			typeIdList.splice(1, 0, 'uint32_t');

			typeList = getTypes(typeIdList, spec.title);
			prefix = 'ptr,num';
		} else {
			ptr = direct;
			prefix = 'ptr';
		}

		// Type ID list was changed.
		dynCall = getDynCall(typeList, spec.title);

		return(buildCallerFunction(
			dynCall,
			null,
			ptr,
			spec.num!,
			spec.policyTbl!,
			needsWireWrite,
			prefix,
			returnType,
			argTypeList
		));
	}

	/* tslint:enable:indent */

	/** Create an overloader that can call several methods with the same name,
	  * depending on the number of arguments passed in the call. */

	export function makeOverloader(func: Func, arity: number) {
		const callerList: FuncList = [];

		function call(this: any) {
			return(callerList[arguments.length].apply(this, arguments));
		}

		(call as any).addMethod = (_func: Func, _arity: number) => {
			callerList[_arity] = _func;
		};

		(call as any).addMethod(func, arity);

		return(call);
	}

	@prepareNamespace('_nbind')
	export class _ {} // tslint:disable-line:class-name
}
