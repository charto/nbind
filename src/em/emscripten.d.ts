// Minimal definitions of the environment available to Emscripten libraries.
// Only contains features required by nbind.

declare var Module: {
	Pointer_stringify(ptr: number, length?: number): string;
	lengthBytesUTF8(str: string): number;
	stringToUTF8Array(
		str: string,
		outU8Array: Uint8Array,
		outIdx: number,
		maxBytesToWrite: number
	): number;
	stringToUTF8(str: string, outPtr: number, maxBytesToWrite: number): number;

	[name: string]: any;
};

declare var Runtime: {
	stackAlloc(size: number): number;
};

declare var _malloc: (size: number) => number;
declare var _free: (ptr: number) => void;

// The HEAP* arrays are the main way to access the C++ heap.

declare var HEAP8: Int8Array;
declare var HEAP16: Int16Array;
declare var HEAP32: Int32Array;
declare var HEAPU8: Uint8Array;
declare var HEAPU16: Uint16Array;
declare var HEAPU32: Uint32Array;
declare var HEAPF32: Float32Array;
declare var HEAPF64: Float64Array;
