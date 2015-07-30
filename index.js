function extend(dst, src) {
	if(src !== null && typeof(src) === 'object') {
		Object.keys(src).reduce(function(dst, key) {
			dst[key] = src[key];
			return(dst);
		}, dst);
	}

	return(dst);
}

module.exports = function(path, args) {
	var nbind = require('bindings')(
		extend({
			module_root: path,
			bindings: 'nbind'
		}, args)
	);

	return(nbind);
};
