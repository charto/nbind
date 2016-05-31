// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// makeModulePathList and findCompiledModule are adapted from the npm module
// "bindings" licensed under the MIT license terms in BINDINGS-LICENSE.

interface NodeRequire {
	(name: string): any;
	(name: 'path'): {
		dirname(path: string): string;
		resolve(...paths: string[]): string;
	};
	resolve(name: string): string;
}

declare var require: NodeRequire;

const path = require('path'); // tslint:disable-line:no-var-requires

declare var process: {
	cwd: () => string,
	env: { [key: string]: string },
	versions: {
		node: string
	},
	platform: string,
	arch: string
};

export interface ModuleSpec {
	type: string;
	name: string;
	path?: string;
}

export type ClassType = { new(...args: any[]): any };

export interface DefaultExportType {
	[ key: string ]: any;

	locateFile?(name: string): string;
	onRuntimeInitialized?(): void;
	ccall?(name: string, returnType?: string, argTypes?: string[], args?: any[]): any;

	_nbind_value?(name: string, proto: ClassType): void;

	NBind?: {
		bind_value(name: string, proto: ClassType): void;
	};
}

export class Binding<ExportType extends DefaultExportType> {
	bind(name: string, proto: ClassType ) {
		if(this.lib._nbind_value) { // emcc
			this.lib._nbind_value(name, proto);
		} else if(this.lib.NBind) {
			this.lib.NBind.bind_value(name, proto);
		}
	}

	binary: ModuleSpec;
	lib: ExportType;
}

let currentBinding: Binding<any>;

/** Called from asm.js during init to get current module. @ignore internal use. */

export function getLib() {
	return(currentBinding.lib);
}

function rethrow(err: any) {
	if(err) throw(err);
}

/** Make list of possible paths for a single compiled output file name. */

function makeModulePathList(root: string, name: string) {
	return([
		// node-gyp's linked version in the "build" dir
		[ root, 'build', name ],

		// node-waf and gyp_addon (a.k.a node-gyp)
		[ root, 'build', 'Debug', name ],
		[ root, 'build', 'Release', name ],

		// Debug files, for development (legacy behavior, remove for node v0.9)
		[ root, 'out', 'Debug', name ],
		[ root, 'Debug', name ],

		// Release files, but manually compiled (legacy behavior, remove for node v0.9)
		[ root, 'out', 'Release', name ],
		[ root, 'Release', name ],

		// Legacy from node-waf, node <= 0.4.x
		[ root, 'build', 'default', name ],

		[
			root,
			process.env['NODE_BINDINGS_COMPILED_DIR'] || 'compiled',
			process.versions.node,
			process.platform,
			process.arch,
			name
		]
	]);
}

export type FindCallback = (err: any, result?: ModuleSpec) => void;

function findCompiledModule(
	root: string,
	specList: ModuleSpec[],
	callback: FindCallback
) {
	const resolvedList: string[] = [];

	for(let spec of specList) {
		// Check if any possible path contains a loadable module,
		// and store unsuccessful attempts.

		for(let pathParts of makeModulePathList(root, spec.name)) {
			const resolvedPath = path.resolve.apply(path, pathParts);

			try {
				spec.path = require.resolve(resolvedPath);

				// Stop if a module was found.
				callback(null, spec);
				return(spec);
			} catch(err) {
				resolvedList.push(resolvedPath);
			}
		}
	}

	const err = new Error(
		'Could not locate the bindings file. Tried:\n' +
		resolvedList.join('\n')
	);

	(err as any).tries = resolvedList;

	callback(err);
	return(null);
}

export function find(cb?: FindCallback): ModuleSpec;
export function find(basePath?: string, cb?: FindCallback): ModuleSpec;

export function find(basePath?: any, cb?: FindCallback) {
	let callback = arguments[arguments.length - 1];
	if(typeof(callback) != 'function') callback = rethrow;

	return(findCompiledModule(
		(basePath != callback && basePath) || process.cwd(), [
			{ name: 'nbind.node', type: 'node' },
			{ name: 'nbind.js',   type: 'emcc' }
		], callback
	));
}

export type InitCallback<ExportType extends DefaultExportType> = (
	err: any,
	result: Binding<ExportType>
) => void;

export function init<ExportType extends DefaultExportType>(
	cb?: InitCallback<ExportType>
): Binding<ExportType>;

export function init<ExportType extends DefaultExportType>(
	basePath?: string,
	cb?: InitCallback<ExportType>
): Binding<ExportType>;

export function init<ExportType extends DefaultExportType>(
	basePath?: string,
	lib?: ExportType,
	cb?: InitCallback<ExportType>
): Binding<ExportType>;

export function init<ExportType extends DefaultExportType>(
	basePath?: any,
	lib?: ExportType,
	cb?: InitCallback<ExportType>
) {
	let callback = arguments[arguments.length - 1];
	if(typeof(callback) != 'function') callback = rethrow;

	const binary = find(basePath != callback && basePath, callback);
	if(!binary) return(null);

	const binding = new Binding<ExportType>();

	binding.binary = binary;
	binding.lib = (lib != callback && lib) || ({} as ExportType);

	if(binary.type == 'emcc') {
		initAsm(binding, callback);
	} else {
		initNode(binding, callback);
	}

	return(binding);
}

function initAsm<ExportType extends DefaultExportType>(
	binding: Binding<ExportType>,
	callback: InitCallback<ExportType>
) {
	const lib = binding.lib;

	lib.locateFile = lib.locateFile || function(name: string) {
		return(path.resolve(path.dirname(binding.binary.path), name));
	};

	const runtimeInitialized = lib.onRuntimeInitialized;

	lib.onRuntimeInitialized = function() {
		if(runtimeInitialized) runtimeInitialized.apply(this, arguments);
		lib.ccall('nbind_init');
		callback(null, binding);
	};

	currentBinding = binding;

	// Load the Asm.js module.
	require(binding.binary.path);
}

function initNode<ExportType extends DefaultExportType>(
	binding: Binding<ExportType>,
	callback: InitCallback<ExportType>
) {
	// Load the compiled addon.
	const lib = require(binding.binary.path);

	if(!lib || typeof(lib) != 'object') {
		throw(new Error('Error loading addon'));
	}

	Object.keys(lib).forEach(function(key: string) {
		binding.lib[key] = lib[key];
	});
}
