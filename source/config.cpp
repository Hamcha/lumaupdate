#include "config.h"

#include <fstream>
#include <algorithm>
#include <functional>
#include <cctype>

static inline void trim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

LoadConfigError Config::LoadFile(const std::string path) {
	std::ifstream cfgfile(path);

	if (!cfgfile.is_open()) {
		return CFGE_NOTEXISTS;
	}

	if (!cfgfile.good()) {
		return CFGE_UNREADABLE;
	}

	std::string curLine;
	while (std::getline(cfgfile, curLine)) {
		size_t index = curLine.find('=');
		if (index != std::string::npos) {
			std::string key = curLine.substr(0, index);
			std::string value = curLine.substr(index + 1);
			trim(key);
			trim(value);
			values[key] = value;
		} else {
			printf("Invalid line detected in config: %s\n", curLine.c_str());
			return CFGE_MALFORMED;
		}
	}

	return CFGE_NONE;
}

bool Config::Has(const std::string key) {
	auto it = values.find(key);
	return it != values.end();
}

std::string Config::Get(const std::string key, const std::string fallback) {
	if (!this->Has(key)) {
		return fallback;
	}
	return values[key];
}