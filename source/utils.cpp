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

	return text;
}

std::string indent(std::string text, const std::string& indent, const size_t cols) {
	const std::string nlindent = "\n" + indent + indent;
	size_t offset = 0;
	bool hasMore = true;

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
			// Find nearest whitespace before cutting point
			size_t cuttingPoint = offset + cols - indent.length();
			size_t distance = 0;
			while (text[cuttingPoint - distance] != ' ') {
				distance++;
				// Give up if the word is too long
				if (distance > 10) {
					distance = 1;
					text.insert(offset + cols - indent.length() - distance, "-");
					distance--;
					break;
				}
			}

			text.insert(offset + cols - indent.length() - distance, nlindent);
			offset += cols + 1;
			extraOffset += nlindent.length();
		}

		offset = index + extraOffset + 1;
	}

	return text;
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

	return text.substr(startIndex, endIndex);
}