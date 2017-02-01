#include "ProgressScreen.h"

#include "../Application.h"
#include "../Utils.h"

#include <cmath>

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
	static const u32 ProgressBoxColorGreen = RGBA8(0, 200, 100, 150);
	static const u32 ProgressBoxColorGrey = RGBA8(140, 190, 170, 100);
	static const float ProgressBoxAnimationOffset = 0.9f;
	static const float ProgressBoxAnimationSpeed = -6.f;
	static const float ProgressBoxAnimationWeight = 0.6f;

	// Draw background first
	sf2d_draw_texture(background, 0, 0);

	// Draw past steps
	for (u8 pastIndex = 0; pastIndex < ProgressPastMax && pastIndex < pastSteps.size(); ++pastIndex) {
		sftd_draw_text(smallfont,
					   ProgressBoxLeft, ProgressTitleHeight - ((pastIndex + 1) * (ProgressPastSize + ProgressPastMargin)),
					   ProgressPastColor, ProgressPastSize, pastSteps[pastSteps.size() - pastIndex - 1].c_str());
	}

	// Draw current step
	sftd_draw_text(bigfont,
				   ProgressBoxLeft, ProgressTitleHeight,
				   ProgressTitleColor, ProgressTitleSize, currentStep.c_str());

	// Draw progress (as squares)
	//// Calculate total bar width so it can be drawn at center
	const u16 screenWidth = sf2d_get_current_screen() == GFX_TOP ? 400 : 320;
	const u16 totalProgressWidth = currentStepProgressCount * (ProgressBoxSize + ProgressBoxMargin) - ProgressBoxMargin;
	const u16 centerBarLeft = (screenWidth - totalProgressWidth) / 2;
	//// Calculate global animation values
	currentAnimationTimer += Application::Get().DeltaTime();

	for (u16 index = 0; index < currentStepProgressCount; ++index) {
		// Calculate horizontal position
		const float horPosition = centerBarLeft + (index * (ProgressBoxSize + ProgressBoxMargin));

		// Get box color
		u32 boxColor = ProgressBoxColorGrey;
		if (index < currentStepProgress) {
			const u8 boxAlpha = ProgressBoxColorGreen >> 24;

			float animationOffset = std::sin(currentAnimationTimer * ProgressBoxAnimationSpeed + (index * ProgressBoxAnimationOffset));
			u8 alphaOffset = (u8)(boxAlpha + animationOffset * ProgressBoxAnimationWeight * 128.f);

			boxColor = (ProgressBoxColorGreen & 0x00ffffff) | (alphaOffset << 24);
		}

		// Draw box
		sf2d_draw_rectangle(horPosition, ProgressBoxHeight, ProgressBoxSize, ProgressBoxSize, boxColor);
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

void ProgressScreen::SetStepProgressInfinite() { SetStepProgress(3, 3); }
