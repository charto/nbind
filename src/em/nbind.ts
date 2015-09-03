/// <reference path="../../node_modules/emscripten-library-decorator/index.ts" />

class _BindType {
	constructor(id: number, name: string) {
		this.id = id;
		this.name = name;
	}

	id: number;
	name: string;
}

interface TypeData {
	bind: (id: number, name: string) => void;
	tbl: { [name: string]: _BindType };
	list: _BindType[];
}

var _typeData: TypeData = {
	bind: (id: number, name: string) => {
		var def = new _BindType(id, name);

		_typeData.tbl[name] = def;
		_typeData.list[id] = def;
	},

	tbl: {},
	list: []
};

class _Wrapper {
	constructor(...args: any[]) {
		console.log(args.length)
		console.log(this.asmConstructor[args.length]);
	}

	asmConstructor: any[] = [];
}

function _readAsciiString(ptr: number) {
	var endPtr = ptr;

	while(HEAPU8[endPtr++]);

	return(String.fromCharCode.apply('', Array.prototype.slice.call(HEAPU8, ptr, endPtr-1)));
}

function _extend(child: any, parent: any) {
	for (var prop in parent) if (parent.hasOwnProperty(prop)) child[prop] = parent[prop];
	function proto() {}
	proto.prototype = parent.prototype;
	child.prototype = new (<any>proto)();
};

@exportLibrary
class nbind {
	static typeData = _typeData;

	@dep(_BindType, 'typeData')
	static _nbind_register_type(id: number, namePtr: number) {
		_typeData.bind(id, _readAsciiString(namePtr));
	}

	@dep(_BindType, 'typeData')
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
			_typeData.bind(idList[num], formatType(flagList[num], sizeList[num]));
		}

		console.log('getTypeID<...>() = ' + idList.join(', '));
		console.log('sizeof(...)      = ' + sizeList.join(', '));
		console.log('flags(...)       = ' + flagList.join(', '));
	}

	@dep(_readAsciiString, _extend, _Wrapper)
	static _nbind_register_class(id: number, namePtr: number) {
		var name = _readAsciiString(namePtr);

		console.error('Register class ' + name);

		_typeData.bind(id, name);

		function foo() {
			_Wrapper.apply(this, arguments);
		}
		_extend(foo, _Wrapper);

		Module[name] = foo;
	}

	@dep(_readAsciiString)
	static _nbind_register_constructor(typeID: number, signaturePtr: number, a: number) {
		var signature = _readAsciiString(signaturePtr);
	}

	@dep(_readAsciiString)
	static _nbind_register_function(namePtr: number, signaturePtr: number) {
		var name = _readAsciiString(namePtr);
		var signature = _readAsciiString(signaturePtr);

		console.error('Register function ' + name + ' type ' + signature);
	}

	@dep(_readAsciiString)
	static _nbind_register_method(namePtr: number, signaturePtr: number) {
		var name = _readAsciiString(namePtr);
		var signature = _readAsciiString(signaturePtr);

		console.error('Register method ' + name + ' type ' + signature);
	}

	@dep('typeData')
	static _nbind_init() {
		console.log(Object.keys(_typeData.tbl).sort().map((name) => {
			return(_typeData.tbl[name].id + '\t' + name);
		}).join('\n'));
	}
};
