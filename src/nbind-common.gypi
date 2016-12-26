{
	"variables": {
		"asmjs%": 0
	},
	"conditions": [
		["asmjs==1", {
			"make_global_settings": [
				["CC",  "<!(npm run -s emcc-path)"],
				["CXX",  "<!(npm run -s emcc-path)"],
				["LINK", "<!(npm run -s emcc-path)"],
			]
		}]
	]
}
