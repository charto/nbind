// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

#pragma once

namespace nbind {

// Wrapper for C++ objects converted from corresponding JavaScript objects.
// Needed for allocating an empty placeholder object before JavaScript calls
// its constructor. See:
// http://stackoverflow.com/questions/31091223/placement-new-return-by-value-and-safely-dispose-temporary-copies

class ArgStorage {

public:

	ArgStorage(unsigned int overloadNum) :
		overloadNum(overloadNum) {}

	unsigned int overloadNum;

};

template <typename Bound>
class TemplatedArgStorage : public ArgStorage {

public:

	TemplatedArgStorage(unsigned int overloadNum) :
		ArgStorage(overloadNum), dummy(0) {}

	~TemplatedArgStorage() {
		if(isValid) data.~Bound();
		isValid = false;
	}

	template <typename... Args>
	void init(Args&&... args) {
		::new(&data) Bound(std::forward<Args>(args)...);
		isValid = true;
	}

	Bound getBound() {
		return(std::move(data));
	}

private:

	union {
		int dummy;
		Bound data;
	};

	bool isValid = false;

};

} // namespace
