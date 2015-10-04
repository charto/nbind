/// <reference path="../../node_modules/emscripten-library-decorator/index.ts" />
/// <reference path="emscripten.d.ts" />

// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// Generic table and list of functions.

type Func = (...args: any[]) => any;
type FuncTbl = { [name: string]: Func };
type FuncList = { (...args: any[]): any }[];
type Invoker = (ptr: number, ...args: any[]) => any;
type TypeIDList = (number | string)[];

// Namespace that will be made available inside Emscripten compiled module.

namespace _nbind {

	// A type definition, which registers itself upon construction.

	export class BindType {
		constructor(id: number, name: string) {
			this.id = id;
			this.name = name;

			// Namespace name is needed here, or TypeScript will mangle it.
			_nbind.typeTbl[name] = this;
			_nbind.typeList[id] = this;
		}

		// TODO: maybe this should be an abstract base class and these versions of wire type conversions
		// should be in a subclass called PrimitiveType.

		needsWireRead: boolean = false;

		makeWireRead(expr: string) {
			return(expr);
		}

		needsWireWrite: boolean = false;

		makeWireWrite(expr: string) {
			return(expr);
		}

		id: number;
		name: string;
	}

	// Base class for wrapped instances of bound C++ classes.

	export class Wrapper {
		free: Func;

		__nbindConstructor: Func;
		__nbindPtr: number;
	}

	// Any subtype (not instance but type) of Wrapper.
	// Declared as anything that constructs something compatible with Wrapper.

	export interface WrapperClass {
		new(...args: any[]): Wrapper;
	}

	// Push a string to the C++ stack, zero-terminated and UTF-8 encoded.

	export function pushCString(str: string) {
		if(str === null || str === undefined) return(0);
		str = str.toString();

		var length = Module.lengthBytesUTF8(str) + 1;
		var result = Runtime.stackAlloc(length);

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
		constructor(id: number, name: string) {
			super(id, name);
		}

		needsWireRead: boolean = true;

		makeWireRead(expr: string) {
			return('_nbind.popCString(' + expr + ')');
		}

		needsWireWrite: boolean = true;

		makeWireWrite(expr: string) {
			return('_nbind.pushCString(' + expr + ')');
		}
	}

	// Booleans are returned as numbers from Asm.js.
	// Prefixing with !! converts them to JavaScript booleans.

	export class BooleanType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		needsWireRead: boolean = true;

		makeWireRead(expr: string) {
			return('!!(' + expr + ')');
		}
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

		return(num);
	}

	// TODO: free the registered callback (and allow persisting it, as needed by value types for example)!

	export class CallbackType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		needsWireWrite: boolean = true;

		makeWireWrite(expr: string) {
			return('_nbind.registerCallback(' + expr + ')');
		}
	}

	export function pushString(str: string) {
		if(str === null || str === undefined) return(0);
		str = str.toString();

		var length = Module.lengthBytesUTF8(str);

		// 32-bit length, string and a zero terminator
		// (stringToUTF8Array insists on adding it)

		var result = Runtime.stackAlloc(4 + length + 1);

		HEAPU32[result / 4] = length;
		Module.stringToUTF8Array(str, HEAPU8, result + 4, length + 1);

		return(result);
	}

	export function popString(ptr: number) {
		if(ptr === 0) return(null);

		var length = HEAPU32[ptr / 4];

		return(Module.Pointer_stringify(ptr + 4, length));
	}

	export class StringType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		needsWireRead: boolean = true;

		makeWireRead(expr: string) {
			return('_nbind.popString(' + expr + ')');
		}

		needsWireWrite: boolean = true;

		makeWireWrite(expr: string) {
			return('_nbind.pushString(' + expr + ')');
		}
	}

	// Special type that constructs a new object.

	export class CreateValueType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		needsWireWrite: boolean = true;

		makeWireWrite(expr: string) {
			return('((_nbind.value=new ' + expr + '),0)');
		}
	}

	// Base class for all bound C++ classes (not their instances),
	// also inheriting from a generic type definition.

	export class BindClass extends BindType {
		constructor(id: number, name: string, proto: WrapperClass) {
			super(id, name);

			this.proto = proto;
		}

		// Reference to JavaScript class for wrapped instances
		// of this C++ class.

		proto: WrapperClass;

		needsWireRead: boolean = true;

		makeWireRead(expr: string) {
			return('(' +
				expr + '||' +
				'_nbind.throwError("Value type JavaScript class is missing or not registered"),' +
				'_nbind.value' +
			')');
		}

		// TODO

		needsWireWrite: boolean = true;

		makeWireWrite(expr: string) {
			return(expr);
		}
	}

	// Look up a list of type objects based on their numeric typeID or name.

	export function getTypes(idList: TypeIDList) {
		return(idList.map((id: number | string) => {
			if(typeof(id) == 'number') return(_nbind.typeList[id as number]);
			else return(_nbind.typeTbl[id as string]);
		}));
	}

	// Generate a mangled signature from argument types.
	// Asm.js functions can only be called though Emscripten-generated invoker functions,
	// with slightly mangled type signatures appended to their names.

	export function makeSignature(typeList: BindType[]) {
		var mangleMap: { [name: string]: string; } = {
			float64_t: 'd',
			float32_t: 'f',
			void: 'v'
		}

		return(typeList.map((type: BindType) => (mangleMap[type.name] || 'i')).join(''));
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
		returnType: BindType,
		argTypeList: BindType[]
	) {
		var argList = makeArgList(argTypeList.length);

		var callExpression = returnType.makeWireRead(
			'dynCall(' +
				[prefix].concat(argList.map(
					(name: string, num: number) => argTypeList[num].makeWireWrite(name)
				)).join(',') +
			')'
		);

		var stackSave = '';
		var stackRestore = '';

		if(needsWireWrite) {
			stackSave = 'var sp=Runtime.stackSave();';
			stackRestore = 'Runtime.stackRestore(sp);';
		}

		var sourceCode = (
			'function(' + argList.join(',') + '){' +
				stackSave +
				'var r=' + callExpression + ';' +
				stackRestore +
				'return r;' +
			'}'
		);

		// console.log(returnType.name + ' func(' + argTypeList.map((type: BindType) => type.name).join(', ') + ')')
		// console.log(sourceCode);

		return(eval('(' + sourceCode + ')'));
	}

	// Check if any type on the list requires conversion.
	// Mainly numbers can be passed as-is between Asm.js and JavaScript.

	function anyNeedsWireWrite(typeList: BindType[]) {
		return(typeList.reduce(
			(result: boolean, type: BindType) =>
				(result || type.needsWireWrite),
			false
		));
	}

	function anyNeedsWireRead(typeList: BindType[]) {
		return(typeList.reduce(
			(result: boolean, type: BindType) =>
				(result || type.needsWireRead),
			false
		));
	}

	export function buildJSCallerFunction(
		returnType: BindType,
		argTypeList: BindType[]
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

		var sourceCode = (
			'function(' + ['dummy', 'num'].concat(argList).join(',') + '){' +
				'var r=' + callExpression + ';' +
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

		if(!returnType.needsWireWrite && !needsWireRead && argCount < 3) switch(argCount) {
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

		if(!returnType.needsWireRead && !needsWireWrite && argCount < 3) switch(argCount) {

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

		if(!returnType.needsWireRead && !needsWireWrite && argCount < 3) switch(argCount) {

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

			idList.splice(1, 0, 'uint32_t');
			signature = makeSignature(getTypes(idList));
			dynCall = Module['dynCall_' + signature];

			return(buildCallerFunction(
				dynCall,
				ptr,
				num,
				needsWireWrite,
				'ptr,num',
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

	export var typeTbl: { [name: string]: BindType } = {};
	export var typeList: BindType[] = [];

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

exportNamespace('_nbind');

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
		}

		new _nbind.BindClass(idList[0], name, Bound);
		new _nbind.BindType(idList[1], name + ' *');
		new _nbind.BindType(idList[2], 'const ' + name + ' *');

		Module[name] = Bound;
	}

	@dep('_nbind')
	static _nbind_register_constructor(typeID: number, typeListPtr: number, typeCount: number, ptr: number) {
		var typeList = Array.prototype.slice.call(HEAPU32, typeListPtr / 4, typeListPtr / 4 + typeCount);

		_nbind.addMethod(
			(_nbind.typeList[typeID] as _nbind.BindClass).proto.prototype,
			'__nbindConstructor',
			_nbind.makeCaller(ptr, 0, ptr, typeList),
			typeCount - 1
		);
	}

	@dep('_nbind')
	static _nbind_register_destructor(typeID: number, ptr: number) {
		_nbind.addMethod(
			(_nbind.typeList[typeID] as _nbind.BindClass).proto.prototype,
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
			(_nbind.typeList[typeID] as _nbind.BindClass).proto as any,
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
		var proto = (_nbind.typeList[typeID] as _nbind.BindClass).proto.prototype;

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
	static nbind_value(name: string, proto: any) {
		Module['NBind'].bind_value(name, proto);
	}
};
