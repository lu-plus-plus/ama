#ifndef AMAL_HPP
#define AMAL_HPP

#include "../script/quickjs/src/quickjs.h"

namespace ama {
	JSValueConst RunScript(char const * pScript, size_t scriptLen, char const * scriptName);
	JSValueConst RunParsing(char const * pSource, char const * const * ppFilters, int filterCount);
	const char * UnwrapStringRaw(JSValueConst val);
	void FreeValue(JSValueConst val);
}

#endif