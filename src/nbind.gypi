{

	"variables": {
		"asmjs%": 0
	},

	"target_name": "nbind",
	"sources": [
		"common.cc",
		"reflect.cc"
	],
	"include_dirs": [
		"../include"
	],

	"conditions": [
		['asmjs==1', {

			"product_name": "nbind.js",
			"type":         "executable",
			"sources":    [ "em/Binding.cc" ],
			"cflags_cc":  [ "<@(_cflags)" ],
			"ldflags":    [ "<@(_cflags)" ],

			"copies": [{"destination": "<(INTERMEDIATE_DIR)", "files": ["pre.js", "post.js", "../dist/em-api.js"]}],
			"prejs_path": "<(INTERMEDIATE_DIR)/pre.js",
			"postjs_path": "<(INTERMEDIATE_DIR)/post.js",
			"jslib_path": "<(INTERMEDIATE_DIR)/em-api.js",

			"cflags": [
				"-O3",
				"-fno-exceptions",
				"--pre-js", "<(_prejs_path)",
				"--post-js", "<(_postjs_path)",
				"--js-library", "<(_jslib_path)",
				"-s", "NO_FILESYSTEM=1",
				"-s", "EXPORTED_FUNCTIONS=[\"_nbind_init\",\"_nbind_value\",\"_nbind_debug\"]",
				"-s", "DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=[\"nbind_value\",\"nbind_debug\",\"\\$$Browser\"]"
			],

            "xcode_settings": {
                "GCC_GENERATE_DEBUGGING_SYMBOLS": "NO",
                "OTHER_CFLAGS": [ "<@(_cflags)" ],
                "OTHER_LDFLAGS": [ "<@(_cflags)" ]
            }

		}, {

			"sources": ["v8/Binding.cc"],
			"cflags_cc": [
				"-O3",
				"-std=c++11",
				"-fexceptions"
			],

			"msbuild_settings": {
				"ClCompile": {
					"RuntimeTypeInfo": "false",
					"ExceptionHandling": "Sync", # /EHsc
					"MultiProcessorCompilation": "true"
				}
			},

			"xcode_settings": {
				"GCC_ENABLE_CPP_EXCEPTIONS": "YES",
				"CLANG_CXX_LANGUAGE_STANDARD":"c++11",
				"MACOSX_DEPLOYMENT_TARGET": "10.7",
				"OTHER_CPLUSPLUSFLAGS": [
				"-O3",
					"-std=c++11",
					"-stdlib=libc++"
				],
				"OTHER_LDFLAGS": ["-stdlib=libc++"]
			}

		}]
	]
}
