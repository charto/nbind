// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

var path = require('path');

// This function is adapted from the npm module "bindings"
// licensed under the MIT license terms in BINDINGS-LICENSE.

function findCompiledModule(root, specList) {
	// Make list of possible paths for a single compiled output file name.

	function makePathList(name) {
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
				process.env.NODE_BINDINGS_COMPILED_DIR || 'compiled',
				process.versions.node,
				process.platform,
				process.arch,
				name
			],
		]);
	}

	var result = null;
	var resolvedList = [];
	var specCount = specList.length;

	for(specNum = 0; specNum < specCount; ++specNum) {
		var spec = specList[specNum];

		var pathList = makePathList(spec.name);
		var pathCount = pathList.length;

		// Check if any path contains a loadable module, and store unsuccessful attempts.

		for(pathNum = 0; pathNum < pathCount; ++pathNum) {
			var resolvedPath = path.resolve.apply(this, pathList[pathNum]);

			try {
				spec.path = require.resolve(resolvedPath);
				result = spec;

				break;
			} catch(err) {
				resolvedList.push(resolvedPath);
			}
		}

		// Stop if a module was found.

		if(pathNum < pathCount) break;
	}

	if(!result) {
		var err = new Error(
			'Could not locate the bindings file. Tried:\n' +
			resolvedList.join('\n')
		);

		err.tries = resolvedList;
		throw(err);
	}

	return(result);
}

var nbind = {
	pendingBindings: {},

	find: function(basePath) {
		return(findCompiledModule(
			basePath || process.cwd(), [
				{ type: 'node', name: 'nbind.node' },
				{ type: 'emcc', name: 'nbind.js' }
			]
		));
	},

	init: function(basePath, lib) {
		var spec = nbind.find(basePath);

		nbind.moduleSpec = spec;
		nbind.lib = lib || {};

		if(spec.type == 'emcc') nbind.initAsm(spec);
		else nbind.initNode(spec);

		return(nbind.lib);
	},

	initAsm: function(spec) {
		nbind.lib.locateFile = function(name) {
			return(path.resolve(path.dirname(spec.path), name));
		}

		// Load the Asm.js module.
		var moduleObj = require(spec.path);

		moduleObj.ccall('nbind_init');

		Object.keys(nbind.pendingBindings).forEach(function(name) {
			nbind.lib._nbind_value(name, nbind.pendingBindings[name]);
		});
	},

	initNode: function(spec) {
		// Load the compiled addon.
		var moduleObj = require(spec.path);

		if(!moduleObj || typeof(moduleObj) != 'object') {
			throw(new Error('Error loading addon'));
		}

		Object.keys(moduleObj).forEach(function(key) {
			nbind.lib[key] = moduleObj[key];
		});

		Object.keys(nbind.pendingBindings).forEach(function(name) {
			nbind.lib.NBind.bind_value(name, nbind.pendingBindings[name]);
		});
	},

	bind: function(name, proto) {
		if(nbind.moduleSpec.type == 'emcc') {
			nbind.lib._nbind_value(name, proto);
		} else if(nbind.lib.NBind) {
			nbind.lib.NBind.bind_value(name, proto);
		} else nbind.pendingBindings[name] = proto;
	}
};

module.exports = nbind;
