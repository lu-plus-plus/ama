//the goal is bug-for-bug node.js compatibility
#include "unicode.hpp"
#include "env.hpp"
#if defined(_WIN32)
	#include <windows.h>
#else
	#include <stdlib.h>
#endif
#include <string>
#include <vector>
#include "jc_array.h"
JC::StringOrError ENV::get(std::span<char> name) {
	#if defined(_WIN32)
		std::vector<uint16_t> name_win = unicode::WTF8ToUTF16(name);
		name_win--->push(int16_t(0));
		auto lg = GetEnvironmentVariableW(LPCWSTR(name_win.data()), nullptr, 0);
		if ( lg == ERROR_ENVVAR_NOT_FOUND ) {
			return nullptr;
		} else {
			std::vector<uint16_t> buf((lg));
			lg = GetEnvironmentVariableW(LPCWSTR(name_win.data()), LPWSTR(buf.data()), lg);
			buf.resize(lg);
			return unicode::UTF16ToUTF8(buf);
		}
	#else
		std::string namei = JC::string_concat(name, char(0));
		char const* ret = getenv(namei.c_str());
		if ( ret ) {
			return std::string(ret);
		} else {
			return nullptr;
		}
	#endif
}
int ENV::set(std::span<char> name, std::span<char> value) {
	#if defined(_WIN32)
		std::vector<uint16_t> name_win = unicode::WTF8ToUTF16(name);
		std::vector<uint16_t> value_win = unicode::WTF8ToUTF16(value);
		name_win--->push(uint16_t(0));
		value_win--->push(uint16_t(0));
		return SetEnvironmentVariableW(LPCWSTR(name_win.data()), LPCWSTR(value_win.data()));
	#else
		std::string namei = JC::string_concat(name, char(0));
		std::string valuei = JC::string_concat(value, char(0));
		return setenv(namei.c_str(), valuei.c_str(), 1) == 0;
	#endif
}
