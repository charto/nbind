{

	"variables": {
		"emcc%": 0
	},

	"target_name": "nbind",
	"include_dirs": [
		"include"
	],

	"conditions": [
		['emcc==1', {

			"product_name": "nbind.js",
			"type":         "executable",
			"sources":    [ "src/em/Binding.cc" ],
			"cflags_cc":  [ "<@(_cflags)" ],
			"ldflags":    [ "<@(_cflags)" ],

			"cflags": [
				"-g",
				"-fno-exceptions",
				"--js-library", "../nbind.js",
				"-s", "NO_FILESYSTEM=1",
				"-s", "EXPORTED_FUNCTIONS=[\"_nbind_init\"]"
			]

		},{

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

		}]
	]
}
