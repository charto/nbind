// This file is part of nbind, copyright (C) 2014-2015 BusFaster Ltd.
// Released under the MIT license, see LICENSE.

// This file handles JavaScript callback functions accessible from C++.

#pragma once

namespace nbind {

class cbOutput {

	template<typename ArgType>
	friend struct BindingType;

public:

	cbOutput(cbFunction &jsConstructor, v8::Local<v8::Value> *output) :
		jsConstructor(jsConstructor), output(output) {}

	// This overload is identical to cbFunction.
	template<typename... Args>
	void operator()(Args&&... args) {
		call<void>(args...);
	}

	template <typename ReturnType, typename... Args>
	void call(Args... args);

private:

	cbFunction &jsConstructor;
	v8::Local<v8::Value> *output;

};

} // namespace
