#include "ProgressScreen.h"

#include <OpenSans_Regular_ttf.h>
#include <loading_bg_png.h>

ProgressScreen::ProgressScreen() {
	// Load fonts
	bigfont = sftd_load_font_mem(OpenSans_Regular_ttf, OpenSans_Regular_ttf_size);
	smallfont = sftd_load_font_mem(OpenSans_Regular_ttf, OpenSans_Regular_ttf_size);

	// Load background
	background = sfil_load_PNG_buffer(loading_bg_png, SF2D_PLACE_RAM);
}

ProgressScreen::~ProgressScreen() {
	// Free fonts
	sftd_free_font(bigfont);
	sftd_free_font(smallfont);

	// Free texture
	sf2d_free_texture(background);
}

void ProgressScreen::Render() {
	static const int ProgressTitleLeft = 20;
	static const int ProgressTitleHeight = 90;
	static const u16 ProgressTitleSize = 20;
	static const u32 ProgressTitleColor = RGBA8(0, 0, 0, 255);
	static const u8  ProgressPastMax = 5;
	static const u16 ProgressPastSize = 15;
	static const u16 ProgressPastMargin = 8;
	static const u32 ProgressPastColor = RGBA8(40, 40, 40, 100);
	static const int ProgressBoxLeft = ProgressTitleLeft + 5;
	static const int ProgressBoxHeight = ProgressTitleHeight + 35;
	static const int ProgressBoxSize = 10;
	static const int ProgressBoxMargin = 10;
	static const u32 ProgressColorGreen = RGBA8(0, 200, 100, 200);
	static const u32 ProgressColorGrey = RGBA8(40, 40, 40, 40);
	// Draw background first
	sf2d_draw_texture(background, 0, 0);

	// Draw past steps
	for (u8 pastIndex = 0; pastIndex < ProgressPastMax && pastIndex < pastSteps.size(); ++pastIndex) {
		sftd_draw_text(smallfont,
					   ProgressTitleLeft, ProgressTitleHeight - ((pastIndex + 1) * (ProgressPastSize + ProgressPastMargin)),
					   ProgressPastColor, ProgressPastSize, pastSteps[pastSteps.size() - pastIndex - 1].c_str());
	}

	// Draw current step
	sftd_draw_text(bigfont,
				   ProgressTitleLeft, ProgressTitleHeight,
				   ProgressTitleColor, ProgressTitleSize, currentStep.c_str());

	// Draw progress (as squares)
	//TODO Improve this
	for (u16 index = 0; index < currentStepProgressCount; ++index) {
		sf2d_draw_rectangle(ProgressBoxLeft + (index * (ProgressBoxSize + ProgressBoxMargin)), ProgressBoxHeight,
							ProgressBoxSize, ProgressBoxSize,
							index < currentStepProgress ? ProgressColorGreen : ProgressColorGrey);
	}
}

void ProgressScreen::SetStepTitle(std::string stepTitle, bool archivePrevious) {
	if (archivePrevious) {
		pastSteps.push_back(currentStep);
	}
	//TODO Animate step transition
	currentStep = stepTitle;
}

void ProgressScreen::SetStepProgress(int currentProgress, int maxProgress) {
	currentStepProgress = currentProgress;
	currentStepProgressCount = maxProgress;
}