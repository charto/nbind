{
	"variables": {
		"asmjs%": 0
	},
	"conditions": [
		["asmjs==1", {
			"make_global_settings": [
				["CXX",  "<!(npm run -s emcc-path)"],
				["LINK", "<!(npm run -s emcc-path)"],
			]
		}]
	]
}
