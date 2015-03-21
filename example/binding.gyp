{
	"targets": [
		{
			"target_name": "example",
			"includes": ["../nbind.gypi"],
			"sources": ["Point.cc"],
			"cflags": ["-std=c++11"],
			"xcode_settings": {
				"MACOSX_DEPLOYMENT_TARGET": "10.7",
				"OTHER_CPLUSPLUSFLAGS":[
					"-std=c++11",
					"-stdlib=libc++"
				],
				"OTHER_LDFLAGS": ["-stdlib=libc++"]
			}
		}
	]
}
