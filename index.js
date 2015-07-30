var pendingBindings = {};

var nbind = {
	init: function(path, args) {
		extend(nbind.module, require('bindings')(
			extend({
				module_root: path,
				bindings: 'nbind'
			}, args)
		));

		Object.keys(pendingBindings).forEach(function(name) {
			nbind.module.NBind.bind(name, pendingBindings[name]);
		});
	},

	bind: function(name, proto) {
		if(nbind.module.NBind) nbind.module.NBind.bind(name, proto);
		else pendingBindings[name] = proto;
	},

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
