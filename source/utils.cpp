#include "utils.h"

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

std::string unescape(const std::string& s)
{
	std::string res;
	auto it = s.begin();
	while (it != s.end())
	{
		char c = *it++;
		if (c == '\\' && it != s.end())
		{
			switch (*it++) {
			case '\\': c = '\\'; break;
			case 'n':  c = '\n'; break;
			case 'r':  c = '\r'; break;
			case 't':  c = '\t'; break;
			case '"':  c = '"' ; break;
			default:   continue;
			}
		}
		res += c;
	}
	return res;
}

std::string stripMarkdown(const std::string& text) {
	//TODO
	return text;
}

std::string indent(std::string text, const std::string& indent, const size_t cols) {
	size_t offset = 0;
	bool hasMore = true;
	std::string nlindent = "\n" + indent + indent;

	while (hasMore) {
		// Start of line, add indent
		text.insert(offset, indent);
		offset += indent.length();

		// Search for new line
		size_t index = text.find('\n', offset + 1);
		if (index == std::string::npos) {
			// Last line, break after this cycle
			hasMore = false;
			index = text.length();
		}

		size_t extraOffset = 0;
		while (index - offset > cols) {
			text.insert(offset + cols - indent.length(), nlindent);
			offset += cols + 1;
			extraOffset += nlindent.length();
		}

		offset = index + extraOffset + 1;
	}

	return text;
}