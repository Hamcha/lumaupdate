#include "config.h"

#include "utils.h"

LoadConfigError Config::LoadFile(const std::string& path) {
	std::ifstream cfgfile(path);

	if (!cfgfile.is_open()) {
		return LoadConfigError::NotExists;
	}

	if (!cfgfile.good()) {
		return LoadConfigError::Unreadable;
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
			logPrintf("Invalid line detected in config: %s\n", curLine.c_str());
			return LoadConfigError::Malformed;
		}
	}

	return LoadConfigError::None;
}

bool Config::Has(const std::string& key) {
	auto it = values.find(key);
	return it != values.end();
}

std::string Config::Get(const std::string& key, const std::string& fallback) {
	if (!this->Has(key)) {
		return fallback;
	}
	return values[key];
}