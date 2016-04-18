#include "utils.h"

#include <sstream>

std::string formatErrMessage(const char* msg, const Result val) {
	std::ostringstream os;
	os << msg << "\nRet code: " << val;
	return os.str();
}