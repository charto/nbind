// This file is a public domain UMD wrapper for nbind asm.js output.
// Returns a wrapper function with parameters:
//   - (Optional) initial asm.js module object, use {} or pass additional options.
//   - Node.js style callback, fires on error or when nbind is ready for use.

// Export wrapper function for different module loaders.
(function(root, wrapper) {
	// AMD.
	if(typeof(define) == 'function' && define.amd) define([], function() { return(wrapper); });
	// Node.js style CommonJS.
	else if(typeof(module) == 'object' && module.exports) module.exports = wrapper;
	// Browser global.
	else (root.nbind = root.nbind || {}).init = wrapper;

// Define wrapper function, the main export from this module.
}(this, function(Module, cb) {
	if(typeof(Module) == 'function') {
		cb = Module;
		Module = {};
	}

	// Set up nbind in asm.js runtime ready hook.
	Module.onRuntimeInitialized = (function(init, cb) {
		return(function() {
			// Call any previous runtime ready hook.
			if(init) init.apply(this, arguments);

			try {
				// Init the C++ side.
				Module.ccall('nbind_init');
			} catch(err) {
				// Report failure.
				cb(err);
				return;
			}

			// Report success through callback passed to wrapper function.
			cb(null, {
				bind: Module._nbind_value,
				reflect: Module.NBind.reflect,
				queryType: Module.NBind.queryType,
				toggleLightGC: Module.toggleLightGC,
				lib: Module
			});
		});
	})(Module.onRuntimeInitialized, cb);

	// Emscripten-generated asm.js code begins.
