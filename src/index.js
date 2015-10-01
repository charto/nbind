// This function is adapted from the npm module "bindings"
// licensed under the MIT license terms in BINDINGS-LICENSE.

function findCompiledModule(root, specList) {
	var Path = require('path');

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
			var resolvedPath = Path.resolve.apply(this, pathList[pathNum]);

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

	init: function(basePath) {
		nbind.moduleSpec = findCompiledModule(
			basePath, [
				{ type: 'node', name: 'nbind.node' },
				{ type: 'emcc', name: 'nbind.js' }
			]
		);

		// Load the compiled addon.

		var moduleObj = require(nbind.moduleSpec.path);

		if(nbind.moduleSpec.type == 'emcc') moduleObj.ccall('nbind_init');

		extend(nbind.module, moduleObj);

		Object.keys(nbind.pendingBindings).forEach(function(name) {
			if(nbind.moduleSpec.type == 'emcc') {
				nbind.module._nbind_value(name, nbind.pendingBindings[name]);
			} else {
				nbind.module.NBind.bind_value(name, nbind.pendingBindings[name]);
			}
		});

		return(this);
	},

	bind: function(name, proto) {
		if(nbind.moduleSpec.type == 'emcc') {
			nbind.module._nbind_value(name, proto);
		} else if(nbind.module.NBind) {
			nbind.module.NBind.bind_value(name, proto);
		} else nbind.pendingBindings[name] = proto;
	},

	moduleSpec: null,

	module: {}
};

function extend(dst, src) {
	if(src !== null && typeof(src) === 'object') {
		Object.keys(src).forEach(function(key) {
			dst[key] = src[key];
		});
	}

	return(dst);
}

module.exports = nbind;
