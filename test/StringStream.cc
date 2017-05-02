#include <sstream>
#include <string>

#include "nbind/api.h"

class StringStream {

public:

	StringStream() {}

	unsigned int bug() {
		auto ss = new std::stringstream();

		(*ss) << "foo";
		(*ss) << "bar";

		unsigned int len = strlen(ss->str().c_str());

		delete ss;

		return(len);
	}

};

#include "nbind/nbind.h"

#ifdef NBIND_CLASS

NBIND_CLASS(StringStream) {
	construct<>();

	method(bug);
}

#endif
