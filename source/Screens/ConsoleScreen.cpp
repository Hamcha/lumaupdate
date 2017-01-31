#include "ConsoleScreen.h"

#include <saxmono_ttf.h>

ConsoleScreen::ConsoleScreen() {
	// Load font
	font = sftd_load_font_mem(saxmono_ttf, saxmono_ttf_size);

	// Create dummy first line
	lines.push_back("");
}

ConsoleScreen::~ConsoleScreen() {
	// Unload font
	sftd_free_font(font);
}

void ConsoleScreen::Render() {
	static const u16 screenHeight = 240;

	// Get maximum width depending on which screen we are drawing on
	const u16 screenWidth = sf2d_get_current_screen() == GFX_TOP ? 400 : 320;
	const u16 actualScreenWidth = screenWidth - (margin * 2);

	// Calculate how many lines to print and where
	u16 currentHeight = margin;
	std::vector<u16> heights;
	for (auto it = lines.rbegin(); it != lines.rend() && currentHeight <= screenHeight - (margin * 2); ++it) {
		int txwidth, txheight;
		sftd_calc_bounding_box(&txwidth, &txheight, font, fontSize, actualScreenWidth, (*it).c_str());
		currentHeight += txheight;
		heights.push_back(txheight);
	}

	// Heights calculated, print each line
	currentHeight = margin;
	u8 lineOffset = lines.size() - heights.size();
	for (u8 index = 0; index < heights.size(); ++index) {
		sftd_draw_text_wrap(font, margin, currentHeight, RGBA8(255, 255, 255, 255), fontSize, actualScreenWidth, lines[lineOffset + index].c_str());
		currentHeight += heights[heights.size() - 1 - index];
	}
}

void ConsoleScreen::Write(const std::string text) {
	// Split content in lines
	for (size_t offset = 0; offset < text.length();) {
		// Get how many characters until newline
		size_t index = text.find_first_of('\n', offset);

		// Get first substring
		std::string currentLine = text.substr(offset, index - offset);

		// Add current line to vector
		lines.back().append(currentLine);

		// If no newlines have been found, exit
		if (index == std::string::npos) {
			break;
		}

		// Otherwise, add a new line, advance offset and continue
		lines.push_back("");
		offset = index + 1;
	}
}

// WriteLine is just an alias to Write + "\n"
void ConsoleScreen::WriteLine(const std::string text) { Write(text + "\n"); }