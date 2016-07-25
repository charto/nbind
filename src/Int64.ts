// This file is part of nbind, copyright (C) 2014-2016 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

function zeroes(count: number) {
	return(new Array(count + 1).join('0'));
}

const padTbl: { [base: number]: string } = {
	2: zeroes(32),
	4: zeroes(16),
	10: zeroes(9),
	16: zeroes(8)
};

/** Simple bignum style class for formatting 64-bit integers. */

export class Int64 {
	constructor(lo: number, hi: number, sign: boolean) {
		this.lo = lo >>> 0;
		this.hi = hi >>> 0;
		this.sign = sign;
	}

	fromJS(output: (lo: number, hi: number, sign: boolean) => void) {
		output(this.lo, this.hi, this.sign);
	}

	toString(base: number) {
		const prefix = this.sign ? '-' : '';
		let hi = this.hi;
		let lo = this.lo;

		if(!base) base = 10;
		if(!hi) return(prefix + lo.toString(base));

		const pad = padTbl[base];
		let part: string;

		if(base != 10) {
			if(!pad) throw(new Error('Unsupported base ' + base));

			part = lo.toString(base);

			return(prefix + hi.toString(base) + pad.substr(part.length) + part);
		}

		const groupSize = 1000000000;
		let result = '';
		let carry: number;

		function step(limb: number) {
			carry = carry * 0x10000 + (limb >>> 16);
			const hi16 = (carry / groupSize) >>> 0;
			carry = carry - hi16 * groupSize;

			carry = carry * 0x10000 + (limb & 0xffff);
			const lo16 = (carry / groupSize) >>> 0;
			carry = carry - lo16 * groupSize;

			return(((hi16 << 16) | lo16) >>> 0);
		}

		while(hi || lo >= groupSize) {
			carry = 0;

			hi = step(hi);
			lo = step(lo);

			part = '' + carry;

			result = pad.substr(part.length) + part + result;
		}

		result = prefix + lo + result;

		return(result);
	}

	lo: number;
	hi: number;
	sign: boolean;
}
