#pragma once

#include "../Screen.h"

class ProgressScreen : public Screen {
public:
	void Render();
	ProgressScreen();
	~ProgressScreen();

	void SetStepTitle(std::string stepTitle, bool archivePrevious = true);
	void SetStepProgress(int currentProgress, int maxProgress = 10);

private:
	sftd_font* font;
	sf2d_texture* background;
	std::string currentStep;
	std::vector<std::string> pastSteps;
	int currentStepProgress = 0, currentStepProgressCount = 0;
};
