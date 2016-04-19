#pragma once

#include <string>
#include <map>

class Config {
private:
	std::map<std::string, std::string> values;

public:
	bool LoadFile(const std::string path);
	bool Has(const std::string key);
	std::string Get(const std::string key);
};