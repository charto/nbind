/// <reference path="../../node_modules/emscripten-library-decorator/index.ts" />
/// <reference path="emscripten.d.ts" />

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

	export class BindMethod {
		invokerSignature: string;
	}

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

	export function pushCString(str: string) {
		if(str === null || str === undefined) return(0);
		str = str.toString();

		var len = Module.lengthBytesUTF8(str) + 1;
		var result = Runtime.stackAlloc(len);

		Module.stringToUTF8Array(str, HEAPU8, result, len);

		return(result);
	}

	export function popCString(ptr: number) {
		if(ptr === 0) return(null);

		return(Module.Pointer_stringify(ptr));
	}

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

	export class BooleanType extends BindType {
		constructor(id: number, name: string) {
			super(id, name);
		}

		needsWireRead: boolean = true;

		makeWireRead(expr: string) {
			return('!!(' + expr + ')');
		}
	}

	// Base class for all bound C++ classes, also inheriting from a generic type definition.

	export class BindClass extends BindType {
		constructor(id: number, name: string, proto: WrapperClass) {
			super(id, name);

			this.proto = proto;
		}

		proto: WrapperClass;
	}

	// Generate mangled signature from argument types.
	// Emscripten generates invoker functions with signatures appended to their names.

	export function getTypes(idList: TypeIDList) {
		return(idList.map((id: number | string) => {
			if(typeof(id) == 'number') return(_nbind.typeList[id as number]);
			else return(_nbind.typeTbl[id as string]);
		}));
	}

	export function makeSignature(typeList: BindType[]) {
		var mangleMap: { [name: string]: string; } = {
			float64_t: 'd',
			float32_t: 'f',
			void: 'v'
		}

		return(typeList.map((type: BindType) => (mangleMap[type.name] || 'i')).join(''));
	}

	function makeArgList(argCount: number) {
		return(
			Array.apply(null, Array(argCount)).map(
				(dummy: any, num: number) => ('a' + (num + 1))
			)
		);
	}

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
			stackSave = 'Runtime.stackSave();';
			stackRestore = 'Runtime.stackRestore();';
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

	function anyNeedsWireWrite(typeList: BindType[]) {
		return(typeList.reduce(
			(result: boolean, type: BindType) =>
				(result || type.needsWireWrite),
			false
		));
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

		var signature = makeSignature(typeList);
		var dynCall = Module['dynCall_' + signature];
		var needsWireWrite = anyNeedsWireWrite(argTypeList);

		if(!returnType.needsWireRead && !needsWireWrite && argCount < 3) switch(argCount) {
			// If there are only a few arguments not requiring type conversion,
			// build a simple invoker function without using eval.

			case 0: return(function() {return(
			        dynCall(ptr, num, this.__nbindPtr));});
			case 1: return(function(                   a1: any) {return(
			        dynCall(ptr, num, this.__nbindPtr, a1       ));});
			case 2: return(function(                   a1: any, a2: any) {return(
			        dynCall(ptr, num, this.__nbindPtr, a1,      a2       ));});
			case 3: return(function(                   a1: any, a2: any, a3: any) {return(
			        dynCall(ptr, num, this.__nbindPtr, a1,      a2,      a3       ));});
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

	export function makeCaller(ptr: number, idList: TypeIDList) {
		var argCount = idList.length - 1;

		var typeList = getTypes(idList);
		var returnType = typeList[0];
		var argTypeList = typeList.slice(1);

		var signature = makeSignature(typeList);
		var dynCall = Module['dynCall_' + signature];
		var needsWireWrite = anyNeedsWireWrite(argTypeList);

		if(!returnType.needsWireRead && !needsWireWrite && argCount < 3) switch(argCount) {
			// If there are only a few arguments not requiring type conversion,
			// build a simple invoker function without using eval.

			case 0: return(() =>
			        dynCall(ptr));
			case 1: return((     a1: any) =>
			        dynCall(ptr, a1       ));
			case 2: return((     a1: any, a2: any) =>
			        dynCall(ptr, a1,      a2       ));
			case 3: return((     a1: any, a2: any, a3: any) =>
			        dynCall(ptr, a1,      a2,      a3       ));
		} else {
			// Function takes over 3 arguments or needs type conversion.
			// Let's create the invoker dynamically then.

			return(buildCallerFunction(
				dynCall,
				ptr,
				0,
				needsWireWrite,
				'ptr',
				returnType,
				argTypeList
			));
		}
	}

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

	export var typeTbl: { [name: string]: BindType } = {};
	export var typeList: BindType[] = [];

	export var MethodType: {
		method: number;
		getter: number;
		setter: number;
	} = {} as any;

	// Export the namespace to Emscripten compiled output.
	// This must be at the end of the namespace!
	// The dummy class _ and everything after it inside the namespace will be discarded,
	// because unfortunately namespaces can't have decorators.

	@exportNamespace('_nbind')
	export class _ {}
}

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

		if(name == 'bool') {
			new _nbind.BooleanType(id, name);
		} else {
			new _nbind.BindType(id, name);
		}
	}

	@dep('_nbind')
	static _nbind_register_types(dataPtr: number) {
		var count = HEAP32[dataPtr/4];
		var idListPtr = HEAP32[dataPtr / 4 + 1] / 4;
		var sizeListPtr = HEAP32[dataPtr / 4 + 2] / 4;
		var flagListPtr = HEAP32[dataPtr / 4 + 3];

		var idList = HEAPU32.subarray(idListPtr, idListPtr + count);
		var sizeList = HEAPU32.subarray(sizeListPtr, sizeListPtr + count);
		var flagList = HEAPU8.subarray(flagListPtr, flagListPtr + count);

		function createType(id: number, flag: number, size: number) {
			var isSignless = flag & 16;
			var isConst =    flag & 8;
			var isPointer =  flag & 4;
			var isFloat =    flag & 2;
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
	static _nbind_register_class(id: number, namePtr: number) {
		var name = _readAsciiString(namePtr);

		class Bound extends _nbind.Wrapper {
			constructor() {
				if(!(this instanceof Bound)) {
					// Apply arguments to the constructor.
					// Few ways to do this work. This one should.
					return(new (Function.prototype.bind.apply(Bound, Array.prototype.concat.apply([null], arguments))));
				}

				super();

				_defineHidden(this.__nbindConstructor.apply(this, arguments))(this, '__nbindPtr');
			}

			@_defineHidden()
			__nbindConstructor: Func;
		}

		new _nbind.BindClass(id, name, Bound);

		Module[name] = Bound;
	}

	@dep('_nbind')
	static _nbind_register_constructor(typeID: number, typeListPtr: number, typeCount: number, ptr: number) {
		var typeList = Array.prototype.slice.call(HEAPU32, typeListPtr / 4, typeListPtr / 4 + typeCount);

		_nbind.addMethod(
			(_nbind.typeList[typeID] as _nbind.BindClass).proto.prototype,
			'__nbindConstructor',
			_nbind.makeCaller(ptr, typeList),
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
	static _nbind_register_function(typeID: number, typeListPtr: number, typeCount: number, ptr: number, namePtr: number) {
		var name = _readAsciiString(namePtr);
		var typeList = Array.prototype.slice.call(HEAPU32, typeListPtr / 4, typeListPtr / 4 + typeCount);

		_nbind.addMethod(
			(_nbind.typeList[typeID] as _nbind.BindClass).proto as any,
			name,
			_nbind.makeCaller(ptr, typeList),
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
};
