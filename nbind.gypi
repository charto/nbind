{
	"target_name": "nbind",
	"include_dirs": [
		"include"
	],
	"sources": ["src/v8/Binding.cc"],
	"cflags_cc": ["-std=c++11", "-fexceptions"],
	"xcode_settings": {
		"GCC_ENABLE_CPP_EXCEPTIONS": "YES",
		"CLANG_CXX_LANGUAGE_STANDARD":"c++11",
		"MACOSX_DEPLOYMENT_TARGET": "10.7",
		"OTHER_CPLUSPLUSFLAGS":[
			"-std=c++11",
			"-stdlib=libc++"
		],
		"OTHER_LDFLAGS": ["-stdlib=libc++"]
	}
}
