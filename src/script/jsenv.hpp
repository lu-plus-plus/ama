#ifndef _JSENV_JCH_HPP
#define _JSENV_JCH_HPP
#include "./quickjs/src/quickjs.h"
#include "../ast/node.hpp"
#include <string>
#include <vector>
#include "../util/jc_array.h"
#include "../util/jc_unique_string.h"
#include <unordered_map>
/*#pragma add("jc_files", "./jsenv.jc");*/
/*#pragma add("h_files", "./quickjs/src/quickjs.h");*/
/*#pragma add("h_files", "./quickjs/src/cutils.h");*/
/*#pragma add("h_files", "./quickjs/src/list.h");*/
/*#pragma add("h_files", "./quickjs/src/quickjs.h");*/
/*#pragma add("h_files", "./quickjs/src/libregexp.h");*/
/*#pragma add("h_files", "./quickjs/src/libregexp-opcode.h");*/
/*#pragma add("h_files", "./quickjs/src/libunicode.h");*/
/*#pragma add("h_files", "./quickjs/src/libunicode-table.h");*/
/*#pragma add("h_files", "./quickjs/src/quickjs-opcode.h");*/
/*#pragma add("h_files", "./quickjs/src/quickjs-atom.h");*/
/*#pragma add("c_files", "./quickjs/src/quickjs.c");*/
/*#pragma add("c_files", "./quickjs/src/libregexp.c");*/
/*#pragma add("c_files", "./quickjs/src/libunicode.c");*/
/*#pragma add("c_files", "./quickjs/src/cutils.c");*/
namespace ama {
	extern JSContext* jsctx;
	extern JSRuntime* g_runtime_handle;
	extern std::string std_module_dir;
	void DumpError(JSContext* ctx);
	static inline JC::unique_string UnwrapString(JSValueConst val) {
		size_t len = int64_t(0uLL);
		char const* ptr = JS_ToCStringLen(ama::jsctx, &len, val);
		return JC::unique_string(ptr, intptr_t(len));
	}
	static inline JSValueConst WrapString(std::span<char> s) {
		return JS_NewStringLen(ama::jsctx, s.data(), s.size());
	}
	static inline JSValueConst WrapStringNullable(JC::unique_string s) {
		return s == nullptr ? JS_NULL : WrapString(s);
	}
	static inline int32_t UnwrapInt32(JSValueConst val, int32_t dflt) {
		int32_t ret = 0;
		if ( JS_IsUndefined(val) || JS_ToInt32(jsctx, &ret, val) < 0 ) {
			ret = dflt;
		}
		return ret;
	}
	extern uint32_t g_node_classid;
	extern JSValue g_node_proto;
	extern std::unordered_map<ama::Node const*, JSValue> g_js_node_map;
	static inline ama::Node* UnwrapNode(JSValueConst val) {
		return (ama::Node*)(JS_GetOpaque(val, g_node_classid));
	}
	JSValueConst WrapNode(ama::Node const* nd);
	static inline JSValueConst WrapNodeArray(std::span<ama::Node*> nds) {
		JSValueConst ret = JS_NewArray(jsctx);
		for (intptr_t i = intptr_t(0L); i < intptr_t(nds.size()); i += 1) {
			JS_SetPropertyUint32(jsctx, ret, uint32_t(uintptr_t(i)), ama::WrapNode(nds[i]));
		}
		return ret;
	}
	static inline std::vector<ama::Node*> UnwrapNodeArray(JSValueConst val) {
		int64_t lg = int64_t(0LL);
		JS_ToInt64(jsctx, &lg, JS_GetPropertyStr(jsctx, val, "length"));
		std::vector<ama::Node*> ret((intptr_t(lg)));
		for (intptr_t i = intptr_t(0L); i < intptr_t(lg); i += 1) {
			ret[i] = UnwrapNode(JS_GetPropertyUint32(jsctx, val, uint32_t(uintptr_t(i))));
		}
		return std::move(ret);
	}
	JC::unique_string FindCommonJSModuleByPath(JC::unique_string fn);
	JC::unique_string FindCommonJSModule(JC::unique_string fn_required, JC::unique_string dir_base);
	extern std::vector<char const*> g_builder_names;
	extern std::vector<char const*> g_node_class_names;
	std::unordered_map<JC::unique_string, int> GetPrioritizedList(JSValueConst options, char const* name);
	extern std::string std_module_dir_global;
	JSValue InheritOptions(JSValueConst options);
};

#endif