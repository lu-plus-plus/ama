#include <string>
#include "../script/jsapi.hpp"
#include "../../modules/cpp/json/json.h"

namespace ama {
	bool AmaLibraryEntry(char const * pTarget, char const * pSource, char const * const * pFilters, int filterCount) {
		std::string extra_script;
		for (int i = 0; i < filterCount; ++i) {
			extra_script += "__pipeline.unshift(", JSON::stringify(JC::string_concat(pFilters[i], ".setup?")), ");\n";
			extra_script += "__pipeline.push(", JSON::stringify(pFilters[i]), ");\n";
		}
		extra_script += "__pipeline.push({full_path: require('path').resolve(", JSON::stringify(pTarget), ")},'Save');\n";
		int err_code = ama::ProcessAmaFile(pSource, extra_script);
		if (err_code == ama::PROCESS_AMA_NOT_FOUND) {
			fprintf(stderr, "unable to load %s\n", pSource);
		} else if (err_code == ama::PROCESS_AMA_EMPTY_SCRIPT) {
			fprintf(stderr, "no script found in %s\n", pSource);
		}
		return err_code == PROCESS_AMA_SUCCESS;
	}
}