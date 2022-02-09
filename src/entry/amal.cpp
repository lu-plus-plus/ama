#include "amal.hpp"

#include <string>

#include "../script/jsapi.hpp"
#include "../../modules/cpp/json/json.h"

namespace ama {

	JSValueConst RunScript(char const * pScript, size_t scriptLen, char const * scriptName) {
		LazyInitScriptEnv();
		return JS_Eval(ama::jsctx, pScript, scriptLen, scriptName, JS_EVAL_TYPE_GLOBAL);
	}

	JSValueConst RunParsing(char const * pSource, char const * const * ppFilters, int filterCount) {
		LazyInitScriptEnv();
		
		// initialize a parsing functor and return it

		std::string scriptBegin = R"---(
			(function(__filename, __code){
				'use strict';
				let __pipeline = GetPipelineFromFilename(__filename);
				let require = __require.bind(null, __filename);
		)---";
		
		std::string scriptEnd = R"---(
				return __global.ParseCode(__code, __pipeline).toSource();
			})
		)---";

		std::string extraScript;
		for (int i = 0; i < filterCount; ++i) {
			extraScript += "__pipeline.splice(1, 0, " + JSON::stringify(JC::string_concat(ppFilters[i], ".setup?")) + ");\n";
			extraScript += "__pipeline.push(" + JSON::stringify(ppFilters[i]) + ");\n";
		}

		std::string script = scriptBegin + extraScript + scriptEnd;

		JSValueConst parsing_func = RunScript(script.data(), script.size(), "parsing_func.js");
		if (JS_IsException(parsing_func)) {
			ama::DumpError(ama::jsctx);
			return parsing_func;
		}

		// return the result from the parsing functor being called upon input

		std::array<JSValueConst, 2> args{
			JS_NewString(ama::jsctx, "parsing.js"),
			JS_NewString(ama::jsctx, pSource)
		};
		JSValueConst result = JS_Call(ama::jsctx, parsing_func, JS_GetGlobalObject(ama::jsctx), args.size(), args.data());
		if (JS_IsException(result)) {
			ama::DumpError(ama::jsctx);
			return result;
		}

		// cleanup

		JS_FreeValue(ama::jsctx, parsing_func);
		for (JSValueConst arg : args)
			JS_FreeValue(ama::jsctx, arg);
		
		return result;
	}

	const char * UnwrapStringRaw(JSValueConst val) {
		size_t len = int64_t(0uLL);
		char const * ptr = JS_ToCStringLen(ama::jsctx, &len, val);
		return ptr;
	}

	void FreeValue(JSValueConst val) {
		JS_FreeValue(ama::jsctx, val);
	}

}