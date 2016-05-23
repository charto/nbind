// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

import {setEvil, dep, exportLibrary, prepareNamespace, publishNamespace, defineHidden} from 'emscripten-library-decorator';
import {_nbind as _resource} from './Resource';
import {_nbind as _type} from './BindingType';

export {_resource, _type};

// Generic table and list of functions.

type Func = (...args: any[]) => any;
type FuncTbl = { [name: string]: Func };
type FuncList = { (...args: any[]): any }[];
type Invoker = (ptr: number, ...args: any[]) => any;
type TypeIDList = (number | string)[];

setEvil((code: string) => eval(code));

var _defineHidden = defineHidden;

// Namespace that will be made available inside Emscripten compiled module.

export namespace _nbind {

	export var resources: typeof _resource.resources;
	export var listResources: typeof _resource.listResources;

	export var BindType: typeof _type.BindType;
	export var BindClass: typeof _type.BindClass;
	export var BooleanType: typeof _type.BooleanType;
	export var CStringType: typeof _type.CStringType;
	export var StringType: typeof _type.StringType;
	export var CallbackType: typeof _type.CallbackType;
	export var CreateValueType: typeof _type.CreateValueType;

	// Base class for wrapped instances of bound C++ classes.

	export class Wrapper {
		free: Func;

		__nbindConstructor: Func;
		__nbindValueConstructor: Func;
		__nbindPtr: number;
	}

	// Any subtype (not instance but type) of Wrapper.
	// Declared as anything that constructs something compatible with Wrapper.

	export interface WrapperClass {
		new(...args: any[]): Wrapper;
	}

	export var callbackSignatureList: Func[] = [];

	// Callbacks are stored in a list, so C++ code can find them by number.
	// A reference count allows storing them in C++ without leaking memory.
	// The first element is a dummy value just so that a valid index to
	// the list always tests as true (useful for the free list implementation).

	export var callbackList: Func[] = [null];
	export var callbackRefCountList: number[] = [0];

	// Free list for recycling available slots in the callback list.

	export var callbackFreeList: number[] = [];

	export function registerCallback(func: Func) {
		if(typeof(func) != 'function') _nbind.throwError('Type mismatch');

		var num = callbackFreeList.pop() || callbackList.length;

		callbackList[num] = func;
		callbackRefCountList[num] = 1;

		return(num);
	}

	export var valueList: any[] = [];

	export var valueFreeList: number[] = [];

	export function storeValue(value: any) {
		var num = valueFreeList.pop() || valueList.length;

		valueList[num] = value;
		return(num);
	}

	// Look up a list of type objects based on their numeric typeID or name.

	export function getTypes(idList: TypeIDList) {
		return(idList.map((id: number | string) => {
			if(typeof(id) == 'number') {
				var type = _nbind.typeList[id as number];
				if(type) return(type);

				var placeholderFlag = HEAPU8[id as number];

				console.log('placeholderFlag = ' + placeholderFlag);
				console.log('ID = ' + id);
for(var i = id as number; i < (id as number) + 10; ++i) console.log(HEAPU8[i]);
				id = HEAPU32[((id as number) >> 2) + 1];
				console.log('ID = ' + id);
				var type = _nbind.typeList[id as number];
				console.log(type);
				if(type) return(type);
			} else return(_nbind.typeTbl[id as string]);
		}));
	}

	// Generate a mangled signature from argument types.
	// Asm.js functions can only be called though Emscripten-generated invoker functions,
	// with slightly mangled type signatures appended to their names.

	export function makeSignature(typeList: _type.BindType[]) {
		var mangleMap: { [name: string]: string; } = {
			float64_t: 'd',
			float32_t: 'f',
			void: 'v'
		}

		return(typeList.map((type: _type.BindType) => (mangleMap[type.name] || 'i')).join(''));
	}

	// Make a list of argument names a1, a2, a3...
	// for dynamically generating function source code.

	function makeArgList(argCount: number) {
		return(
			Array.apply(null, Array(argCount)).map(
				(dummy: any, num: number) => ('a' + (num + 1))
			)
		);
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

	// Add a method to a C++ class constructor (for static methods) or prototype,
	// or overload an existing method.

	export function addMethod(obj: FuncTbl, name: string, func: Func, arity: number) {
		var overload = obj[name] as any;

		// Check if the function has been overloaded.

		if(overload) {
			if(overload.arity || overload.arity === 0) {
				// Found an existing function, but it's not an overloader.
				// Make a new overloader and add the existing function to it.

				overload = makeOverloader(overload, overload.arity);
				obj[name] = overload;
			}

			// Add this function as an overload.

			overload.addMethod(func, arity);
		} else {
			// Add a new function and store its arity in case it gets overloaded.

			(func as any).arity = arity;

			obj[name] = func;
		}
	}

	// Mapping from numeric typeIDs and type names to objects with type information.

	export var typeTbl: { [name: string]: _type.BindType } = {};
	export var typeList: _type.BindType[] = [];

	// Enum specifying if a method is a getter or setter or not.

	export var MethodType: {
		method: number;
		getter: number;
		setter: number;
	} = {} as any;

	export var value: any;

	export function throwError(message: string) {
		throw({ message: message });
	}

	// Export the namespace to Emscripten compiled output.
	// This must be at the end of the namespace!
	// The dummy class is needed because unfortunately namespaces can't have decorators.
	// Everything after it inside the namespace will be discarded.

	@prepareNamespace('_nbind')
	export class _ {}
}

publishNamespace('_nbind');

function _readAsciiString(ptr: number) {
	var endPtr = ptr;

	while(HEAPU8[endPtr++]);

	return(String.fromCharCode.apply('', HEAPU8.subarray(ptr, endPtr-1)));
}

@exportLibrary
class nbind {
	@dep('_nbind')
	static _nbind_register_method_getter_setter_id(methodID: number, getterID: number, setterID: number) {
		_nbind.MethodType.method = methodID;
		_nbind.MethodType.getter = getterID;
		_nbind.MethodType.setter = setterID;
	}

	@dep('_nbind')
	static _nbind_register_type(id: number, namePtr: number) {
		var name = _readAsciiString(namePtr);

		// TODO: convert this into a lookup table.

		if(name == 'bool') {
			new _nbind.BooleanType(id, name);
		} else if(name == 'cbFunction &') {
			new _nbind.CallbackType(id, name);
		} else if(name == 'std::string') {
			new _nbind.StringType(id, name);
		} else if(name == '_nbind_new') {
			new _nbind.CreateValueType(id, name);
		} else {
			new _nbind.BindType(id, name);
		}
	}

	@dep('_nbind')
	static _nbind_register_types(dataPtr: number) {
		var count       = HEAPU32[dataPtr / 4];
		var idListPtr   = HEAPU32[dataPtr / 4 + 1] / 4;
		var sizeListPtr = HEAPU32[dataPtr / 4 + 2] / 4;
		var flagListPtr = HEAPU32[dataPtr / 4 + 3];

		var idList   = HEAPU32.subarray(idListPtr,   idListPtr   + count);
		var sizeList = HEAPU32.subarray(sizeListPtr, sizeListPtr + count);
		var flagList = HEAPU8. subarray(flagListPtr, flagListPtr + count);

		function createType(id: number, flag: number, size: number) {
			var isSignless = flag & 16;
			var isConst    = flag & 8;
			var isPointer  = flag & 4;
			var isFloat    = flag & 2;
			var isUnsigned = flag & 1;

			var name = [].concat([
				isConst && 'const '
			], ( flag & 20 ?
				[
					!isSignless && (isUnsigned ? 'un' : '') + 'signed ',
					'char'
				]
			:
				[
					isUnsigned && 'u',
					isFloat ? 'float' : 'int',
					size * 8 + '_t'
				]
			), [
				isPointer && ' *'
			]).filter((x: any) => (x as boolean)).join('');

			if(isPointer) {
				new _nbind.CStringType(id, name);
			} else {
				new _nbind.BindType(id, name);
			}
		}

		for(var num = 0; num < count; ++num) {
			createType(idList[num], flagList[num], sizeList[num]);
		}
	}

	@dep('_nbind', _readAsciiString, '__extends')
	static _nbind_register_class(idListPtr: number, namePtr: number) {
		var name = _readAsciiString(namePtr);
		var idList = HEAPU32.subarray(idListPtr / 4, idListPtr / 4 + 3);

		class Bound extends _nbind.Wrapper {
			constructor() {
				// super() never gets called here but TypeScript 1.8 requires it.
				if((false && super()) || !(this instanceof Bound)) {

					// Constructor called without new operator.
					// Make correct call with given arguments.
					// Few ways to do this work. This one should.
					// See http://stackoverflow.com/questions/1606797/use-of-apply-with-new-operator-is-this-possible

					return(new (Function.prototype.bind.apply(
						Bound, // arguments.callee
						Array.prototype.concat.apply([null], arguments)
					)));
				}

				super();

				_defineHidden(this.__nbindConstructor.apply(this, arguments))(this, '__nbindPtr');
			}

			@_defineHidden()
			__nbindConstructor: Func;

			@_defineHidden()
			__nbindValueConstructor: Func;
		}

		new _nbind.BindClass(idList[0], name, Bound);
		new _nbind.BindType(idList[1], name + ' *');
		new _nbind.BindType(idList[2], 'const ' + name + ' *');

		Module[name] = Bound;
	}

	@dep('_nbind')
	static _nbind_register_constructor(
		typeID: number,
		typeListPtr: number,
		typeCount: number,
		ptr: number,
		ptrValue: number
	) {
		var typeList = Array.prototype.slice.call(HEAPU32, typeListPtr / 4, typeListPtr / 4 + typeCount);

		var proto = (_nbind.typeList[typeID] as _type.BindClass).proto.prototype;

		_nbind.addMethod(
			proto,
			'__nbindConstructor',
			_nbind.makeCaller(ptr, 0, ptr, typeList),
			typeCount - 1
		);

		// First argument is a pointer to the C++ object to construct in place.
		// It fits in an unsigned int...

		typeList.splice(0, 1, 'void', 'uint32_t');

		_nbind.addMethod(
			proto,
			'__nbindValueConstructor',
			_nbind.makeCaller(ptrValue, 0, ptrValue, typeList),
			typeCount
		);
	}

	@dep('_nbind')
	static _nbind_register_destructor(typeID: number, ptr: number) {
		_nbind.addMethod(
			(_nbind.typeList[typeID] as _type.BindClass).proto.prototype,
			'free',
			_nbind.makeMethodCaller(ptr, 0, typeID, ['void']),
			0
		);
	}

	@dep('_nbind', _readAsciiString)
	static _nbind_register_function(
		typeID: number,
		typeListPtr: number,
		typeCount: number,
		ptr: number,
		namePtr: number,
		num: number,
		direct: number
	) {
		var name = _readAsciiString(namePtr);
		var typeList = Array.prototype.slice.call(HEAPU32, typeListPtr / 4, typeListPtr / 4 + typeCount);

		_nbind.addMethod(
			(_nbind.typeList[typeID] as _type.BindClass).proto as any,
			name,
			_nbind.makeCaller(ptr, num, direct, typeList),
			typeCount - 1
		);
	}

	@dep('_nbind', _readAsciiString)
	static _nbind_register_method(
		typeID: number,
		typeListPtr: number,
		typeCount: number,
		ptr: number,
		namePtr: number,
		num: number,
		methodType: number
	) {
		var name = _readAsciiString(namePtr);
		var typeList = Array.prototype.slice.call(HEAPU32, typeListPtr / 4, typeListPtr / 4 + typeCount);
		var proto = (_nbind.typeList[typeID] as _type.BindClass).proto.prototype;

		if(methodType == _nbind.MethodType.method) {
			_nbind.addMethod(
				proto,
				name,
				_nbind.makeMethodCaller(ptr, num, typeID, typeList),
				typeCount - 1
			);

			return;
		}

		// The C++ side gives the same name to getters and setters.
		var prefixMatcher = /^[Gg]et_?([A-Z]?)/;

		name = name.replace(prefixMatcher, (match: string, initial: string) => initial.toLowerCase());

		if(methodType == _nbind.MethodType.setter) {

			// A setter is always followed by a getter, so we can just
			// temporarily store an invoker in the property.
			// The getter definition then binds it properly.

			proto[name] = _nbind.makeMethodCaller(ptr, num, typeID, typeList);
		} else {
			Object.defineProperty(proto, name, {
				get: _nbind.makeMethodCaller(ptr, num, typeID, typeList),
				set: proto[name],
				enumerable: true,
				configurable: true
			});
		}
	}

	@dep('_nbind')
	static _nbind_register_callback_signature(
		typeListPtr: number,
		typeCount: number
	) {
		var typeList = Array.prototype.slice.call(HEAPU32, typeListPtr / 4, typeListPtr / 4 + typeCount);
		var num = _nbind.callbackSignatureList.length;

		_nbind.callbackSignatureList[num] = _nbind.makeJSCaller(typeList);

		return(num);
	}

	@dep('_nbind')
	static _nbind_get_value_object(num: number, ptr: number) {
		var obj = _nbind.valueList[num];

		_nbind.valueList[num] = null;

		_nbind.valueFreeList.push(num);

		obj.fromJS(function() {
			obj.__nbindValueConstructor.apply(this, Array.prototype.concat.apply([ptr], arguments));
		});
	}

	@dep('_nbind')
	static _nbind_reference_callback(num: number) {
		++_nbind.callbackRefCountList[num];
	}

	@dep('_nbind')
	static _nbind_free_callback(num: number) {
		if(--_nbind.callbackRefCountList[num] == 0) {
			_nbind.callbackList[num] = null;

			_nbind.callbackFreeList.push(num);
		}
	}

	@dep('_nbind')
	static nbind_value(name: string, proto: any) {
		Module['NBind'].bind_value(name, proto);

		// Copy value constructor reference from C++ wrapper prototype
		// to equivalent JS prototype.

		_defineHidden(
			(_nbind.typeTbl[name] as _type.BindClass).proto.prototype.__nbindValueConstructor
		)(proto.prototype, '__nbindValueConstructor');
	}

	@dep('_nbind')
	static nbind_debug() {}

};
