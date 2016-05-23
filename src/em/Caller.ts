// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {setEvil, prepareNamespace} from 'emscripten-library-decorator';
import {_nbind as _globals} from './Globals';
import {_nbind as _type} from './BindingType';
import {_nbind as _resource} from './Resource';

setEvil((code: string) => eval(code));

export namespace _nbind {

	type Func = _globals.Func;
	type FuncList = _globals.FuncList;
	type TypeIDList = _globals.TypeIDList;

	export var getTypes: typeof _globals.getTypes;
	export var makeSignature: typeof _globals.makeSignature;
	export var callbackList: typeof _globals.callbackList;

	export var listResources: typeof _resource.listResources;

	// Make a list of argument names a1, a2, a3...
	// for dynamically generating function source code.

	function makeArgList(argCount: number) {
		return(
			Array.apply(null, Array(argCount)).map(
				(dummy: any, num: number) => ('a' + (num + 1))
			)
		);
	}

	// Check if any type on the list requires conversion.
	// Mainly numbers can be passed as-is between Asm.js and JavaScript.

	function anyNeedsWireWrite(typeList: _type.BindType[]) {
		return(typeList.reduce(
			(result: boolean, type: _type.BindType) =>
				(result || type.needsWireWrite),
			false
		));
	}

	function anyNeedsWireRead(typeList: _type.BindType[]) {
		return(typeList.reduce(
			(result: boolean, type: _type.BindType) =>
				(result || type.needsWireRead),
			false
		));
	}

	// Dynamically build and evaluate source code for a function that calls
	// an Asm.js invoker function with appropriate type conversion, pushing
	// complicated types to the stack and restoring it afterwards if necessary.

	function buildCallerFunction(
		dynCall: Func,
		ptr: number,
		num: number,
		needsWireWrite: boolean,
		prefix: string,
		returnType: _type.BindType,
		argTypeList: _type.BindType[]
	) {
		var argList = makeArgList(argTypeList.length);

		var callExpression = returnType.makeWireRead(
			'dynCall(' +
				[prefix].concat(argList.map(
					(name: string, num: number) => argTypeList[num].makeWireWrite(name)
				)).join(',') +
			')'
		);

		var resourceSet = listResources([returnType].concat(argTypeList));

		var sourceCode = (
			'function(' + argList.join(',') + '){' +
				resourceSet.open +
				'var r=' + callExpression + ';' +
				resourceSet.close +
				'return r;' +
			'}'
		);

		// console.log(returnType.name + ' func(' + argTypeList.map((type: BindType) => type.name).join(', ') + ')')
		// console.log(sourceCode);

		return(eval('(' + sourceCode + ')'));
	}

	export function buildJSCallerFunction(
		returnType: _type.BindType,
		argTypeList: _type.BindType[]
	) {
		var argList = makeArgList(argTypeList.length);

		var callbackList = _nbind.callbackList;

		var callExpression = returnType.makeWireWrite(
			'callbackList[num](' +
				argList.map(
					(name: string, num: number) => argTypeList[num].makeWireRead(name)
				).join(',') +
			')'
		);

		var resourceSet = listResources([returnType].concat(argTypeList));

		var sourceCode = (
			'function(' + ['dummy', 'num'].concat(argList).join(',') + '){' +
				resourceSet.open +
				'var r=' + callExpression + ';' +
				resourceSet.close +
				'return r;' +
			'}'
		);

		// console.log(returnType.name + ' func(' + argTypeList.map((type: BindType) => type.name).join(', ') + ')')
		// console.log(sourceCode);

		return(eval('(' + sourceCode + ')'));
	}

	// Dynamically create an invoker function for a JavaScript function.

	export function makeJSCaller(idList: TypeIDList) {
		var argCount = idList.length - 1;

		var typeList = getTypes(idList);
		var returnType = typeList[0];
		var argTypeList = typeList.slice(1);
		var needsWireRead = anyNeedsWireRead(argTypeList);

		if(!returnType.needsWireWrite && !needsWireRead && argCount <= 3) switch(argCount) {
			case 0: return(function(dummy: number, num: number) {
			                    return(callbackList[num](    ));});
			case 1: return(function(dummy: number, num: number, a1: any) {
			                    return(callbackList[num](       a1    ));});
			case 2: return(function(dummy: number, num: number, a1: any, a2: any) {
			                    return(callbackList[num](       a1,      a2    ));});
			case 3: return(function(dummy: number, num: number, a1: any, a2: any, a3: any) {
			                    return(callbackList[num](       a1,      a2,      a3    ));});
		} else {
			return(buildJSCallerFunction(
				returnType,
				argTypeList
			));
		}
	}

	// Dynamically create an invoker function for a C++ class method.

	export function makeMethodCaller(ptr: number, num: number, boundID: number, idList: TypeIDList) {
		var argCount = idList.length - 1;

		// The method invoker function has two additional arguments compared to the method itself:
		// Target object and number of the method in a list of methods with identical signatures.

		idList.splice(1, 0, 'uint32_t', boundID);

		var typeList = getTypes(idList);
		var returnType = typeList[0];
		var argTypeList = typeList.slice(3);
		var needsWireWrite = anyNeedsWireWrite(argTypeList);

		var signature = makeSignature(typeList);
		var dynCall = Module['dynCall_' + signature];

		if(!returnType.needsWireRead && !needsWireWrite && argCount <= 3) switch(argCount) {

			// If there are only a few arguments not requiring type conversion,
			// build a simple invoker function without using eval.

			case 0: return(function() {return(
			        dynCall(ptr, num, this.__nbindPtr));});
			case 1: return(function(                   a1: any) {return(
			        dynCall(ptr, num, this.__nbindPtr, a1    ));});
			case 2: return(function(                   a1: any, a2: any) {return(
			        dynCall(ptr, num, this.__nbindPtr, a1,      a2    ));});
			case 3: return(function(                   a1: any, a2: any, a3: any) {return(
			        dynCall(ptr, num, this.__nbindPtr, a1,      a2,      a3    ));});
		} else {

			// Function takes over 3 arguments or needs type conversion.
			// Let's create the invoker dynamically then.

			return(buildCallerFunction(
				dynCall,
				ptr,
				num,
				needsWireWrite,
				'ptr,num,this.__nbindPtr',
				returnType,
				argTypeList
			));
		}
	}

	// Dynamically create an invoker function for a C++ function.

	export function makeCaller(ptr: number, num: number, direct: number, idList: TypeIDList) {
		var argCount = idList.length - 1;

		var typeList = getTypes(idList);
		var returnType = typeList[0];
		var argTypeList = typeList.slice(1);
		var needsWireWrite = anyNeedsWireWrite(argTypeList);

		var signature = makeSignature(typeList);
		var dynCall = Module['dynCall_' + signature];

		if(!returnType.needsWireRead && !needsWireWrite && argCount <= 3) switch(argCount) {

			// If there are only a few arguments not requiring type conversion,
			// build a simple invoker function without using eval.

			case 0: return(() =>
			        dynCall(direct));
			case 1: return((        a1: any) =>
			        dynCall(direct, a1       ));
			case 2: return((        a1: any, a2: any) =>
			        dynCall(direct, a1,      a2       ));
			case 3: return((        a1: any, a2: any, a3: any) =>
			        dynCall(direct, a1,      a2,      a3       ));
		} else {

			// Function takes over 3 arguments or needs type conversion.
			// Let's create the invoker dynamically then.

			// If there's a dispatcher that doesn't call the function directly,
			// pass the num argument to it.

			var prefix = 'ptr';

			if(ptr != direct) {
				idList.splice(1, 0, 'uint32_t');
				prefix = 'ptr,num';
			}

			signature = makeSignature(getTypes(idList));
			dynCall = Module['dynCall_' + signature];

			return(buildCallerFunction(
				dynCall,
				ptr,
				num,
				needsWireWrite,
				prefix,
				returnType,
				argTypeList
			));
		}
	}

	// Create an overloader that can call several methods with the same name,
	// depending on the number of arguments passed in the call.

	export function makeOverloader(func: Func, arity: number) {
		var callerList: FuncList = [];

		var call = function call() {
			return(callerList[arguments.length].apply(this, arguments));
		} as any;

		call.addMethod = (func: Func, arity: number) => {
			callerList[arity] = func;
		}

		call.addMethod(func, arity);

		return(call);
	}

	@prepareNamespace('_nbind')
	export class _ {}
}
