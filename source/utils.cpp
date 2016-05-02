#include "utils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>
#include <sstream>

std::string formatErrMessage(const char* msg, const Result& val) {
	std::ostringstream os;
	os << msg << "\nRet code: " << val;
	return os.str();
}

bool fileExists(const std::string& path) {
	std::ifstream file(path);
	bool isok = file.is_open();
	file.close();
	return isok;
}

void trim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}