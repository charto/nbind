#pragma once

#ifdef EMSCRIPTEN

#define NBIND 1

namespace nbind {
	typedef void (*funcPtr)();
}

#include <type_traits>
#include <forward_list>
#include <vector>
#include <cstring>

template<typename...> struct TypeList {};

extern "C" {
	extern void _nbind_register_class(const char *msg);
	extern void _nbind_register_function(const char *msg);
	extern void _nbind_register_method(const char *msg);
}

#include "BindClass.h"

namespace nbind {

class Bindings {

public:

	static void registerClass(BindClassBase *bindClass);
//	static void initModule(v8::Handle<v8::Object> exports);
//	static void setValueConstructorByName(const char *name, cbFunction &func);

	static inline const char *getError() {return(message);}
	static inline void clearError() {Bindings::message = nullptr;}
	static inline void setError(const char *message) {
		if(!Bindings::message) Bindings::message = message;
	}

private:

	static const char *message;

	// Linkage for a list of all C++ class wrappers.

	static std::forward_list<BindClassBase *> &getClassList() {
		static std::forward_list<BindClassBase *> classList;
		return(classList);
	}

};

} // namespace

#include "BindDefiner.h"

#endif // EMSCRIPTEN
