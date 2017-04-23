
var op = process.argv[2].match(/^([gl])([te])$/);
var wanted = (process.argv[3] || '').split('.');
var version = process.versions.node.split('.');

if(!op || wanted.length != 3) {
	console.log([
		'usage:',
		process.argv[0].replace(/.*\//, ''),
		process.argv[1].replace(/.*\//, ''),
		'op',
		'x.y.z'
	].join(' '));

	console.log('\nCompare Node.js version.');
	console.log('Following op codes determine whether it must be:\n');

	console.log('\tgt\tGreater than x.y.z.');
	console.log('\tge\tGreater than or equal to x.y.z.');
	console.log('\tlt\tLess than x.y.z.');
	console.log('\tle\tLess than or equal to x.y.z.');

	process.exit(1);
}

for(var i = 0; i < 3; ++i) {
	if(+version[i] > +wanted[i]) process.exit(+(op[1] != 'g'));
	if(+version[i] < +wanted[i]) process.exit(+(op[1] == 'g'));
}

process.exit(+(op[2] != 'e'));
