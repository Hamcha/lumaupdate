#include "utils.h"
#include <cmath>
#include <stdarg.h>

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

std::string unescape(const std::string& s) {
	std::string res;
	auto it = s.begin();
	while (it != s.end()) {
		char c = *it++;
		if (c == '\\' && it != s.end()) {
			switch (*it++) {
			case '\\': c = '\\'; break;
			case 'n':  c = '\n'; break;
			case 'r':  c = '\r'; break;
			case 't':  c = '\t'; break;
			case '"':  c = '"'; break;
			default:   continue;
			}
		}
		res += c;
	}
	return res;
}

std::string stripMarkdown(std::string text) {
	// Strip links
	size_t offset = 0;
	while (true) {
		size_t index = text.find('[', offset);
		if (index == std::string::npos) {
			// No more open square brackets, exit
			break;
		}

		if (index != 0 && text[index - 1] == '\\') {
			// Bracket is escaped, skip
			offset = index + 1;
			continue;
		}

		// Get link name
		size_t closing = text.find(']', index + 1);
		if (closing == std::string::npos) {
			// None? Not a link then. Skip to next one
			offset = index + 1;
			continue;
		}

		std::string linkName = text.substr(index + 1, closing - index - 1);

		// Get link end
		size_t linkEnd = text.find(')', closing + 1);
		if (linkEnd == std::string::npos) {
			// None? Not a link then. Skip to next one
			offset = closing + 1;
			continue;
		}

		// Replace everything with the link name
		text.replace(index, linkEnd - index + 1, linkName);
	}

	// Strip \r
	offset = 0;
	while (true) {
		offset = text.find('\r', offset);
		if (offset == std::string::npos) {
			// No more \r, exit
			break;
		}

		// Replace with blank
		text.replace(offset, 1, "");
	}

	return text;
}

std::string indent(const std::string& text, const size_t cols) {
	static const std::string indent = " ";
	static const std::string nlindent = indent + indent;

	std::string out = "";
	size_t offset = 0;
	bool hasMore = true;

	while (hasMore) {
		size_t index = text.find('\n', offset);
		if (index == std::string::npos) {
			// Last line, break after this cycle
			hasMore = false;
			index = text.length();
		}

		bool fstline = true;
		while (index - offset >= cols - 2) {
			size_t textamt = cols - 2;
			textamt -= (fstline ? indent.length() : nlindent.length());

			// Search for nearby whitespace (word wrapping)
			size_t lastWhitespace = text.find_last_of(' ', offset + textamt);
			size_t distance = (offset + cols) - lastWhitespace;
			if (lastWhitespace != std::string::npos && distance < 15) {
				// Nearby space found, wrap word to next line
				std::string curline = text.substr(offset, lastWhitespace - offset);
				out += (fstline ? indent : nlindent) + curline + "\n";
				textamt = lastWhitespace - offset + 1;
			} else {
				// No nearby space found, truncate word
				std::string curline = text.substr(offset, textamt);
				out += (fstline ? indent : nlindent) + curline + "-\n";
			}
			offset += textamt;
			fstline = false;
		}

		out += (fstline ? indent : nlindent) + text.substr(offset, index - offset) + "\n";
		offset = index + 1;
	}

	return out;
}

int getPageCount(const std::string& text, const int rows) {
	return ceil(std::count(text.begin(), text.end(), '\n') / (float)rows);
}

std::string getPage(const std::string& text, const int num, const int rows) {
	const int startAtIndex = rows * num;
	const int stopAtIndex = rows * (num + 1);

	std::size_t startIndex = 0;
	for (int i = 0; i < startAtIndex; i++) {
		startIndex = text.find('\n', startIndex + 1);
		if (startIndex == std::string::npos) {
			return "";
		}
	}

	std::size_t endIndex = startIndex;
	for (int i = startAtIndex; i < stopAtIndex; i++) {
		endIndex = text.find('\n', endIndex + 1);
		if (endIndex == std::string::npos) {
			break;
		}
	}

	return text.substr(startIndex == 0 ? 0 : startIndex + 1, endIndex - startIndex);
}

FILE* _logfile = nullptr;

void logInit(const char* path) {
	_logfile = fopen(path, "w+");
}
void logExit() {
	if (_logfile != nullptr) {
		fclose(_logfile);
	}
}

void logPrintf(const char* format, ...) {
	va_list args;
	va_start(args, format);
	if (_logfile != nullptr) {
		vfprintf(_logfile, format, args);
	}
	vprintf(format, args);
	va_end(args);
}