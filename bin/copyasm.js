#!/usr/bin/env node

var fs = require('fs');
var path = require('path');
var mkdirp = require('mkdirp');

var nbind = require('../dist/nbind.js');

function printUsage() {
	console.log([
		'',
		'  Usage: ' + path.basename(process.argv[1], '.js') + ' [source] target',
		'',
		'  Copy compiled asm.js binaries to desired location',
		'',
		'      source    Path to directory containing binding.gyp',
		'                (node-gyp directs compiler output inside).',
		'                Default is current working directory.',
		'',
		'      target    Path to an existing directory to copy',
		'                .js and .js.mem files into.',
		''
	].join('\n'));
}

var cwd = process.cwd();
var sourcePath, targetPath;

switch(process.argv.length) {
	default:
		printUsage();
		process.exit(1);

	case 3:
		sourcePath = cwd;
		targetPath = path.resolve(cwd, process.argv[2]);
		break;

	case 4:
		sourcePath = path.resolve(cwd, process.argv[2]);
		targetPath = path.resolve(cwd, process.argv[3]);
		break;
}

var binaryPath = nbind.find(sourcePath).path;
var memPath = binaryPath + '.mem';

var pathList = [ binaryPath ];

try {
	fs.statSync(memPath);
	pathList.push(memPath);
} catch(err) {}

console.log([
	'',
	'Copying:',
	pathList.map(function(absolutePath) {
		return('    ' + path.relative(cwd, absolutePath));
	}).join('\n'),
	'to:',
	'    ' + targetPath,
	''
].join('\n'));

mkdirp(targetPath, function(err) {
	if(err) {
		console.error(err);
		return;
	}

	pathList.forEach(function(srcPath) {
		var dstPath = path.join(targetPath, path.basename(srcPath));

		fs.createReadStream(srcPath).pipe(fs.createWriteStream(dstPath));
	});
})
