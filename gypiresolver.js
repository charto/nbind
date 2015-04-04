module.exports = function(moduleNameList) {
	return(moduleNameList.map(function(moduleName) {
		return(require.resolve(moduleName));
	}));
};
