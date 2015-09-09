/// <reference path="../../node_modules/emscripten-library-decorator/index.ts" />

namespace _nbind {
	export class BindType {
		constructor(id: number, name: string) {
			this.id = id;
			this.name = name;

			// Namespace name is needed here, or TypeScript will mangle it.
			_nbind.typeTbl[name] = this;
			_nbind.typeList[id] = this;
		}

		id: number;
		name: string;
	}

	export class BindMethod {
		invokerSignature: string;
	}

	export class BindClass extends BindType {
		constructor(id: number, name: string, proto: Wrapper) {
			super(id, name);

			this.proto = proto;
		}

		proto: Wrapper;
	}

	export class Wrapper {
		__nbindConstructor: any[];
		__nbindPtr: number;
	}

	export function makeSignature(typeList: number[]) {
		var mangleMap: { [name: string]: string; } = {
			float64_t: 'd',
			float32_t: 'f',
			void: 'v'
		}

		return(typeList.map((id: number) => (mangleMap[_nbind.typeList[id].name] || 'i')).join(''));
	}

	export function makeMethodCaller(dynCall: (ptr: number, ...args: any[]) => any, ptr: number, num: number, argCount: number) {
		switch(argCount) {
			case 0: return(function() {return(
			        dynCall(ptr, num, this.__nbindPtr));});
			case 1: return(function(                   a1: any) {return(
			        dynCall(ptr, num, this.__nbindPtr, a1       ));});
			case 2: return(function(                   a1: any, a2: any) {return(
			        dynCall(ptr, num, this.__nbindPtr, a1,      a2       ));});
			case 3: return(function(                   a1: any, a2: any, a3: any) {return(
			        dynCall(ptr, num, this.__nbindPtr, a1,      a2,      a3       ));});
			default:
				// Function takes over 3 arguments.
				// Let's create the invoker dynamically then.

				var argList = Array.apply(null, Array(argCount)).map((x: any, i: number) => ('a' + (i + 1)));

				return(eval('(function(' + argList + '){return(dynCall(ptr,num,this.__nbindPtr,' + argList + '));})'));
		}
	}

	export function makeCaller(dynCall: (ptr: number, ...args: any[]) => any, ptr: number, argCount: number) {
		switch(argCount) {
			case 0: return(() =>
			        dynCall(ptr));
			case 1: return((     a1: any) =>
			        dynCall(ptr, a1       ));
			case 2: return((     a1: any, a2: any) =>
			        dynCall(ptr, a1,      a2       ));
			case 3: return((     a1: any, a2: any, a3: any) =>
			        dynCall(ptr, a1,      a2,      a3       ));
			default:
				// Function takes over 3 arguments.
				// Let's create the invoker dynamically then.

				var argList = Array.apply(null, Array(argCount)).map((x: any, i: number) => ('a' + (i + 1)));

				return(eval('(function(' + argList + ') {return(dynCall(ptr,' + argList + '));})'));
		}
	}

	export var typeTbl: { [name: string]: BindType } = {};
	export var typeList: BindType[] = [];

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
	static _nbind_register_type(id: number, namePtr: number) {
		new _nbind.BindType(id, _readAsciiString(namePtr));
	}

	@dep('_nbind')
	static _nbind_register_types(dataPtr: number) {
		var count = HEAP32[dataPtr/4];
		var idListPtr = HEAP32[dataPtr / 4 + 1] / 4;
		var sizeListPtr = HEAP32[dataPtr / 4 + 2] / 4;
		var flagListPtr = HEAP32[dataPtr / 4 + 3];

		var idList = Array.prototype.slice.call(HEAPU32, idListPtr, idListPtr + count);
		var sizeList = Array.prototype.slice.call(HEAPU32, sizeListPtr, sizeListPtr + count);
		var flagList = Array.prototype.slice.call(HEAPU8, flagListPtr, flagListPtr + count);

		function formatType(flag: number, size: number) {
			var isSignless = flag & 16;
			var isConst =    flag & 8;
			var isPointer =  flag & 4;
			var isFloat =    flag & 2;
			var isUnsigned = flag & 1;

			return([].concat([
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
			]).filter((x: any) => (<boolean>x)).join(''));
		}

		for(var num = 0; num < count; ++num) {
			new _nbind.BindType(idList[num], formatType(flagList[num], sizeList[num]));
		}
	}

	@dep('_nbind', _readAsciiString, '__extends')
	static _nbind_register_class(id: number, namePtr: number) {
		var name = _readAsciiString(namePtr);

		function Bound(...args: any[]) {
			var create = this.__nbindConstructor[args.length];

			if(create) _defineHidden(create.apply(this, arguments))(this, '__nbindPtr');

			// TODO: remove this debug statement, which gets an already constructed object's address as a parameter to the constructor.
			else _defineHidden(args[0])(this, '__nbindPtr');
		}

		// Make "Bound" a subclass of "Wrapper".
		__extends(Bound, _nbind.Wrapper);

//		_defineHidden([])(Bound.prototype, '__nbindConstructor');
Bound.prototype.__nbindConstructor=[];

		new _nbind.BindClass(id, name, <_nbind.Wrapper><any>Bound);

		Module[name] = Bound;
	}

	static _nbind_register_constructor(typeID: number, ptr: number, typeListPtr: number, typeCount: number) {
		var typeList = Array.prototype.slice.call(HEAPU32, typeListPtr / 4, typeListPtr / 4 + typeCount);
		var signature = _nbind.makeSignature(typeList);

		var constructorList = (<any>(<_nbind.BindClass>_nbind.typeList[typeID]).proto).prototype.__nbindConstructor;
	}

	@dep('_nbind', _readAsciiString)
	static _nbind_register_function(typeID: number, ptr: number, namePtr: number, typeListPtr: number, typeCount: number) {
		var name = _readAsciiString(namePtr);
		var typeList = Array.prototype.slice.call(HEAPU32, typeListPtr / 4, typeListPtr / 4 + typeCount);
		var signature = _nbind.makeSignature(typeList);

		(<any>(<_nbind.BindClass>_nbind.typeList[typeID]).proto)[name] =
			_nbind.makeCaller(Module['dynCall_' + signature], ptr, typeCount - 1);
	}

	@dep('_nbind', _readAsciiString)
	static _nbind_register_method(typeID: number, ptr: number, num: number, namePtr: number, typeListPtr: number, typeCount: number) {
		var name = _readAsciiString(namePtr);
		var typeList = Array.prototype.slice.call(HEAPU32, typeListPtr / 4, typeListPtr / 4 + typeCount);

		// The method invoker function has two additional arguments compared to the method itself:
		// Target object and number of the method in a list of methods with identical signatures.

		typeList.splice(1, 0, _nbind.typeTbl['uint32_t'].id, typeID)

		var signature = _nbind.makeSignature(typeList);

		(<any>(<_nbind.BindClass>_nbind.typeList[typeID]).proto).prototype[name] =
			_nbind.makeMethodCaller(Module['dynCall_' + signature], ptr, num, typeCount - 1);
	}
};
