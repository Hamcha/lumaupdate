#include "libs.h"

#include "arnutil.h"
#include "autoupdate.h"
#include "config.h"
#include "http.h"
#include "console.h"
#include "update.h"
#include "release.h"
#include "utils.h"
#include "version.h"

#define WAIT_START while (aptMainLoop() && !(hidKeysDown() & KEY_START)) { gspWaitForVBlank(); hidScanInput(); }

bool redraw = false;
Config config;

/* States */

enum UpdateState {
	UpdateConfirmationScreen,
	Updating,
	UpdateComplete,
	UpdateFailed,
	Restoring,
	RestoreComplete,
	RestoreFailed
};

enum class ChoiceType {
	NoChoice,
	UpdatePayload,
	RestoreBackup
};

enum class SelfUpdateChoice {
	NoChoice,
	SelfUpdate,
	IgnoreUpdate,
};

struct UpdateChoice {
	ChoiceType type          = ChoiceType::NoChoice;
	ReleaseVer chosenVersion = ReleaseVer{};
	bool       isHourly      = false;

	explicit UpdateChoice(const ChoiceType type)
		:type(type) {}
	UpdateChoice(const ChoiceType type, const ReleaseVer& ver, const bool hourly)
		:type(type), chosenVersion(ver), isHourly(hourly) {}
};

struct UpdateInfo {
	// Detected options
	std::string  currentVersion;
	std::string  backupVersion;
	bool         migrateARN     = false;
	bool         backupExists   = false;

	// Configuration options
	std::string  payloadPath    = "/arm9loaderhax.bin";
	bool         backupExisting = true;
	bool         selfUpdate     = true;

	// Available data
	ReleaseInfo* stable = nullptr;
	ReleaseInfo* hourly = nullptr;

	// Chosen settings
	UpdateChoice choice = UpdateChoice(ChoiceType::NoChoice);

	UpdateArgs getArgs() {
		return UpdateArgs{ payloadPath, backupExisting, migrateARN, choice.chosenVersion, choice.isHourly };
	}
};

struct PromptStatus {
	// Redraw queries
	bool redrawTop = false;
	bool redrawBottom = false;
	bool partialredraw = false;
	bool redrawRequired() { return redrawTop || redrawBottom || partialredraw; }
	void resetRedraw() { redrawTop = redrawBottom = partialredraw = false; }

	// Selection and paging
	int  selected = 0;
	int  currentPage = 0;
	int  pageCount = 0;

	// Prompt choice taken?
	bool optionChosen = false;
};

static inline int drawChangelog(const std::string& name, const std::string& log, const int page) {
	int pageCount = 0;

	consoleScreen(GFX_BOTTOM);
	consoleClear();
	consoleMoveTo(1, 1);

	if (log != "") {
		// Get full text
		std::string releaseNotes = indent(stripMarkdown(log), 39);

		// Get page count
		pageCount = getPageCount(releaseNotes, 23);

		// Print header
		printf("%sRelease notes for %s%s%s\n\n", CONSOLE_YELLOW, CONSOLE_GREEN, name.c_str(), CONSOLE_RESET);

		if (page > 0) {
			consoleMoveTo(18, 2);
			printf("....\n");
		}

		// Get current page and print it
		std::string releasePage = getPage(releaseNotes, page, 23);
		printf("%s", releasePage.c_str());

		if (page < pageCount - 1) {
			consoleMoveTo(18, 26);
			printf("....");
		}

		consoleMoveTo(2, 28);

		if (pageCount > 1) {
			std::printf("L R  prev/next          Page %d of %d", page + 1, pageCount);
		}
	} else {
		printf("%sNo release notes found for %sv%s%s\n\n", CONSOLE_YELLOW, CONSOLE_GREEN, name.c_str(), CONSOLE_RESET);
	}

	return pageCount;
}

static inline void handlePromptInput(PromptStatus& status) {
	const u32 keydown = hidKeysDown();

	if (keydown & KEY_UP) {
		status.partialredraw = true;
		--status.selected;
	}
	if (keydown & KEY_DOWN) {
		status.partialredraw = true;
		++status.selected;
	}

	if (keydown & KEY_L && status.currentPage > 0) {
		--status.currentPage;
		status.redrawBottom = true;
	}
	if (keydown & KEY_R && status.currentPage < status.pageCount - 1) {
		++status.currentPage;
		status.redrawBottom = true;
	}

	status.optionChosen = keydown & KEY_A;
}

static UpdateChoice drawConfirmationScreen(const UpdateInfo& args, const bool usingConfig) {
	static PromptStatus status;
	static int hourlyOptionStart = INT_MAX;
	static int extraOptionStart = INT_MAX;

	const std::string latestStable = versionGetStable(args.currentVersion);
	const std::string latestCommit = versionGetCommit(args.currentVersion);

	const bool isDev = args.currentVersion.find("(dev)") != std::string::npos;

	const bool haveLatestStable = latestStable == args.stable->name;
	const bool haveLatestCommit = latestCommit == args.hourly->commits[isDev ? "dev hourly" : "hourly"];

	const bool backupVersionDetected = args.backupExists && args.backupVersion != "";

	handlePromptInput(status);

	if (status.optionChosen) {
		if (status.selected < hourlyOptionStart) {
			return UpdateChoice(ChoiceType::UpdatePayload, args.stable->versions[status.selected], false);
		}
		if (status.selected < extraOptionStart) {
			return UpdateChoice(ChoiceType::UpdatePayload, args.hourly->versions[status.selected - hourlyOptionStart], true);
		}
		int extraOptionID = status.selected - extraOptionStart;

		// Skip restoring backups if they don't exist
		if (!args.backupExists) {
			extraOptionID++;
		}

		switch (extraOptionID) {
		case 0:
			// Restore backup
			return UpdateChoice(ChoiceType::RestoreBackup);
		default:
			// Panic!
			printf("Unknown option selected (?)\n");
			WAIT_START;
			redraw = true;
			status.selected = 0;
			return UpdateChoice(ChoiceType::NoChoice);
		}
	}

	if (redraw) {
		status.redrawTop = status.redrawBottom = true;
	}

	if (!status.redrawRequired()) {
		return UpdateChoice(ChoiceType::NoChoice);
	}

	consoleScreen(GFX_TOP);

	if (status.redrawTop) {
		consoleClear();
		consolePrintHeader();

		if (!usingConfig) {
			std::printf("  %sConfiguration not found, using default values%s\n\n", CONSOLE_MAGENTA, CONSOLE_RESET);
		}

		std::printf("  Payload path:   %s%s%s\n", CONSOLE_WHITE, args.payloadPath.c_str(), CONSOLE_RESET);
		std::printf("  Backup payload: %s%s%s\n\n",
			(args.backupExisting ? CONSOLE_GREEN : CONSOLE_RED),
			(args.backupExisting ? "Yes" : "No"),
			CONSOLE_RESET
		);

		if (args.currentVersion != "") {
			std::printf("  Current installed version: %s%s%s\n", (haveLatestStable ? CONSOLE_GREEN : CONSOLE_RED), args.currentVersion.c_str(), CONSOLE_RESET);
		} else {
			std::printf("  %sCould not detect current version%s\n\n", CONSOLE_MAGENTA, CONSOLE_RESET);
		}
		if (backupVersionDetected) {
			bool backupIsLatest = args.backupVersion == args.stable->name;
			std::printf("  Current backup version:    %s%s%s\n", (backupIsLatest ? CONSOLE_GREEN : CONSOLE_RED), args.backupVersion.c_str(), CONSOLE_RESET);
		}
		std::printf("  Latest version (Github):   %s%s%s\n", CONSOLE_GREEN, args.stable->name.c_str(), CONSOLE_RESET);

		if (args.hourly != nullptr) {
			std::printf("  Latest hourly build:       %s%s%s\n", CONSOLE_GREEN, args.hourly->name.c_str(), CONSOLE_RESET);
		}

		if (haveLatestStable) {
			if (haveLatestCommit || latestCommit == "") {
				std::printf("\n  You seem to have the latest version already.\n");
			} else {
				std::printf("\n  A new hourly build of Luma3DS is available.\n");
			}
		} else {
			std::printf("\n  A new stable version of Luma3DS is available.\n");
		}

		consolePrintFooter();
	}

	if (status.redrawBottom) {
		status.pageCount = drawChangelog("v" + args.stable->name, args.stable->description, status.currentPage);
		consoleScreen(GFX_TOP);
	}

	int y = 11 + (!usingConfig ? 3 : 0) + (args.hourly != nullptr ? 1 : 0) + (backupVersionDetected ? 1 : 0);

	consoleMoveTo(0, y);

	// Wrap around cursor
	int optionCount = args.stable->versions.size() + (args.hourly != nullptr ? args.hourly->versions.size() : 0) + (args.backupExists ? 1 : 0);
	while (status.selected < 0) status.selected += optionCount;
	status.selected = status.selected % optionCount;

	// Print options
	int curOption = 0;
	for (ReleaseVer r : args.stable->versions) {
		std::printf("     Install %s\n", r.friendlyName.c_str());
		++curOption;
	}

	hourlyOptionStart = curOption;
	if (args.hourly != nullptr) {
		for (ReleaseVer h : args.hourly->versions) {
			std::printf("     Install %s\n", h.friendlyName.c_str());
			++curOption;
		}
	}

	extraOptionStart = curOption;

	// Extra #0: Restore backup
	if (args.backupExists) {
		std::printf("     Restore backup\n");
		++curOption;
	}

	// Print cursor
	consoleMoveTo(3, y + status.selected);
	std::printf("\x10");

	// Reset redraw vars
	redraw = false;
	status.resetRedraw();
	return UpdateChoice(ChoiceType::NoChoice);
}

static SelfUpdateChoice drawUpdateNag(const LatestUpdaterInfo& latest) {
	static PromptStatus status;

	handlePromptInput(status);

	if (status.optionChosen) {
		switch (status.selected) {
		case 0:
			// Self update
			return SelfUpdateChoice::SelfUpdate;
		case 1:
			// Ignore update
			return SelfUpdateChoice::IgnoreUpdate;
		default:
			consoleScreen(GFX_TOP);
			// Panic!
			printf("Unknown option selected (?)\n");
			WAIT_START;
			redraw = true;
			status.selected = 0;
			return SelfUpdateChoice::NoChoice;
		}
	}

	if (redraw) {
		status.redrawTop = status.redrawBottom = true;
	}

	if (!status.redrawRequired()) {
		return SelfUpdateChoice::NoChoice;
	}

	if (status.redrawTop) {
		consoleScreen(GFX_TOP);
		consoleClear();

		consoleMoveTo(5, 10);
		std::printf("%sAn update for LumaUpdater is available!%s", CONSOLE_YELLOW, CONSOLE_RESET);

		consoleMoveTo(5, 12);
		std::printf("LumaUpdater %s%s%s", CONSOLE_GREEN, latest.version.c_str(), CONSOLE_RESET);
		consoleMoveTo(5, 13);
		std::printf("See screen below for changes", CONSOLE_GREEN, latest.version.c_str(), CONSOLE_RESET);

		consoleMoveTo(5, 15);
		std::printf("Choose action:");

		consolePrintFooter();
	}

	if (status.redrawBottom) {
		status.pageCount = drawChangelog(latest.version, latest.changelog, status.currentPage);
		consoleScreen(GFX_TOP);
	}

	// Wrap around cursor
	status.selected = status.selected % 2;

	// Print options
	consoleMoveTo(6, 17);
	std::printf("  Update (must restart afterwards)");
	consoleMoveTo(6, 18);
	std::printf("  Don't update and continue");

	// Print cursor
	consoleMoveTo(6, 17 + status.selected);
	std::printf("\x10");

	// Reset redraw vars
	redraw = false;
	status.resetRedraw();
	return SelfUpdateChoice::NoChoice;
}

int main(int argc, char* argv[]) {
	std::vector<std::string> cfgPaths = {
		"/lumaupdater.cfg",
		"/3DS/lumaupdater.cfg",
		"/luma/lumaupdater.cfg",
	};

	UpdateState state = UpdateConfirmationScreen;
	ReleaseInfo release = {}, hourly = {};
	UpdateInfo updateInfo = {};
	UpdateResult result;

	aptInit();
	amInit();
	gfxInitDefault();
	httpcInit(0);

	consoleInitEx();

	consoleScreen(GFX_TOP);
	consoleInitProgress("Loading Luma3DS Updater", "Reading configuration file", 0);
	consoleScreen(GFX_BOTTOM);

	// If argv0 is present, add its path (without file) to config paths
	if (argc > 0) {
		const std::string path(argv[0]);
		size_t start = path.find_first_of(':') + 1;
		const std::string realpath = path.substr(start, path.find_last_of('/') - start);
		cfgPaths.push_back(realpath + "/lumaupdater.cfg");
	}

	// Read config file
	bool configFound = false;
	for (const std::string& path : cfgPaths) {
		const LoadConfigError confStatus = config.LoadFile(path);
		switch (confStatus) {
		case CFGE_NOTEXISTS:
			break;
		case CFGE_UNREADABLE:
			std::printf("FATAL\nConfiguration file is unreadable!\n\nPress START to quit.\n");
			gfxFlushBuffers();
			WAIT_START
			goto cleanup;
		case CFGE_MALFORMED:
			std::printf("FATAL\nConfiguration file is malformed!\n\nPress START to quit.\n");
			gfxFlushBuffers();
			WAIT_START
			goto cleanup;
		case CFGE_NONE:
			std::printf("Configuration file loaded successfully.\n");
			configFound = true;
			break;
		}
		if (configFound) {
			break;
		}
	}

	if (!configFound) {
		std::printf("The configuration file could not be found, skipping...\n");
	} else {
		// Check required values in config, if existing
		if (!config.Has("payload path")) {
			std::printf("Missing required config value: payload path\n");
			gfxFlushBuffers();
			WAIT_START
			goto cleanup;
		}
	}

	// Load config values
	updateInfo.payloadPath = config.Get("payload path", PAYLOADPATH);
	updateInfo.backupExisting = tolower(config.Get("backup", "y")[0]) == 'y';
	updateInfo.selfUpdate = tolower(config.Get("selfupdate", "y")[0]) == 'y';

	// Add initial slash to payload path, if missing
	if (updateInfo.payloadPath[0] != '/') {
		updateInfo.payloadPath = "/" + updateInfo.payloadPath;
	}

	// Check that the payload path is valid
	if (updateInfo.payloadPath.length() > MAXPATHLEN) {
		std::printf("\nFATAL\nPayload path is too long!\nIt can contain at most %d characters!\n\nPress START to quit.\n", MAXPATHLEN);
		gfxFlushBuffers();
		WAIT_START
		goto cleanup;
	}

	if (updateInfo.selfUpdate) {
		consoleScreen(GFX_TOP);
		consoleSetProgressData("Checking for an updated updater", 0.1);
		consoleScreen(GFX_BOTTOM);

		consoleScreen(GFX_TOP);
		consoleSetProgressData("Detecting updater install", 0.2);
		consoleScreen(GFX_BOTTOM);

		// Check for selfupdate
		std::printf("Trying detection of current updater install...\n");
		UpdaterInfo info = { HomebrewType::Unknown, HomebrewLocation::Unknown };
		bool selfupdateContinue = true;

		info = updaterGetInfo(argc > 0 ? argv[0] : nullptr);
		gfxFlushBuffers();

		if (info.type == HomebrewType::Unknown) {
			std::printf("Could not detect install type, skipping self-update...\n");
			selfupdateContinue = false;
		}

		if (info.location == HomebrewLocation::Remote) {
			std::printf("Updater launched over 3DSLink, skipping self-update...\n");
			selfupdateContinue = false;
		}


		if (selfupdateContinue) {
			std::printf("Checking for new Luma3DS Updater releases...\n");
			LatestUpdaterInfo newUpdater;
			try {
				newUpdater = updaterGetLatest();
				selfupdateContinue = newUpdater.isNewer;
				if (!newUpdater.isNewer) {
					std::printf("Current updater is already at latest release.\n");
				}
			} catch (const std::string& err) {
				std::printf("Got error: %s\nSkipping self-update...\n", err.c_str());
				selfupdateContinue = false;
			}

			if (selfupdateContinue) {
				redraw = true;

				// Show selfupdate nag
				bool choiceMade = false;
				while (aptMainLoop() && !choiceMade) {
					hidScanInput();
					u32 kDown = hidKeysDown();
					
					switch (drawUpdateNag(newUpdater)) {
					case SelfUpdateChoice::IgnoreUpdate:
						selfupdateContinue = false;
						choiceMade = true;
						break;
					case SelfUpdateChoice::SelfUpdate:
						selfupdateContinue = true;
						choiceMade = true;
						break;
					case SelfUpdateChoice::NoChoice:
						choiceMade = false;
						break;
					};

					if ((kDown & KEY_START) != 0) {
						goto cleanup;
					}

					// Flush and swap framebuffers
					gfxFlushBuffers();
					gfxSwapBuffers();
					gspWaitForVBlank();
				}
			}

			consoleScreen(GFX_TOP);
			consoleInitProgress("Loading Luma3DS Updater");
		}
		consoleScreen(GFX_BOTTOM);
		consoleClear();
	} else {
		std::printf("Skipping self-update checks as it's disabled\n");
	}

	consoleScreen(GFX_TOP);
	consoleSetProgressData("Detecting installed version", 0.3);
	consoleScreen(GFX_BOTTOM);

	// Try to detect current version
	std::printf("Trying detection of current payload version...\n");
	updateInfo.currentVersion = versionMemsearch(updateInfo.payloadPath);

	// Detect bak version, if exists
	if (fileExists(updateInfo.payloadPath + ".bak")) {
		updateInfo.backupExists = true;
		updateInfo.backupVersion = versionMemsearch(updateInfo.payloadPath + ".bak");
	}

	// Check for eventual migration from ARN to Luma
	updateInfo.migrateARN = arnVersionCheck(updateInfo.currentVersion);

	consoleScreen(GFX_TOP);
	consoleSetProgressData("Fetching latest release data", 0.6);
	consoleScreen(GFX_BOTTOM);

	updateInfo.stable = nullptr;
	try {
		release = releaseGetLatestStable();
		updateInfo.stable = &release;
	} catch (const std::runtime_error& e) {
		std::printf("%s\n", e.what());
		std::printf("\nFATAL ERROR\nFailed to obtain required data.\n\nPress START to exit.\n");
		gfxFlushBuffers();
		WAIT_START
		goto cleanup;
	}

	consoleScreen(GFX_TOP);
	consoleSetProgressData("Fetching latest hourly", 0.8);
	consoleScreen(GFX_BOTTOM);

	updateInfo.hourly = nullptr;
	try {
		hourly = releaseGetLatestHourly();
		updateInfo.hourly = &hourly;
	} catch (const std::runtime_error& e) {
		std::printf("%s\n", e.what());
		std::printf("\nWARN\nCould not obtain latest hourly, skipping...\n");
		gfxFlushBuffers();
	}

	consoleClear();
	consoleScreen(GFX_TOP);
	redraw = true;

	// Main loop
	while (aptMainLoop()) {
		hidScanInput();
		u32 kDown = hidKeysDown();

		switch (state) {
		case UpdateConfirmationScreen:
			updateInfo.choice = drawConfirmationScreen(updateInfo, configFound);
			switch (updateInfo.choice.type) {
			case ChoiceType::UpdatePayload:
				state = Updating;
				redraw = true;
				break;
			case ChoiceType::RestoreBackup:
				state = Restoring;
				redraw = true;
				break;
			}
			break;
		case Updating:
			result = update(updateInfo.getArgs());
			state = result.success ? UpdateComplete : UpdateFailed;
			redraw = true;
			break;
		case UpdateFailed:
			if (redraw) {
				consoleScreen(GFX_TOP);
				consoleClear();
				consolePrintHeader();
				std::printf("\n  %sUpdate failed%s.\n\n  " \
					"Something went wrong while trying to update," \
					"\n  see screen below for details.\n\n  " \
					"Reason for failure: %s\n\n  "
					"If you think this is a bug, please open an\n  " \
					"issue on the following URL:\n  https://github.com/Hamcha/lumaupdate/issues\n\n  " \
					"Press START to exit.\n", CONSOLE_RED, CONSOLE_RESET, result.errcode.c_str());
				redraw = false;
			}
			break;
		case UpdateComplete:
			if (redraw) {
				consoleClear();
				consolePrintHeader();
				std::printf("\n  %sUpdate complete.%s\n", CONSOLE_GREEN, CONSOLE_RESET);
				if (updateInfo.backupExisting) {
					std::printf("\n  In case something goes wrong you can restore\n  the old payload from %s.bak\n", updateInfo.payloadPath.c_str());
				}
				std::printf("\n  Press START to reboot.");
				redraw = false;
			}
			if ((kDown & KEY_START) != 0) {
				// Reboot!
				aptOpenSession();
				APT_HardwareResetAsync();
				aptCloseSession();
			}
			break;
		case Restoring:
			result = restore(updateInfo.getArgs());
			state = result.success ? RestoreComplete : RestoreFailed;
			redraw = true;
			break;
		case RestoreComplete:
			if (redraw) {
				consoleClear();
				consolePrintHeader();
				std::printf("\n  %sRestore complete.%s\n" \
					"\n  Press START to reboot.",
					CONSOLE_GREEN, CONSOLE_RESET);
				redraw = false;
			}
			if ((kDown & KEY_START) != 0) {
				// Reboot!
				aptOpenSession();
				APT_HardwareResetAsync();
				aptCloseSession();
			}
			break;
		case RestoreFailed:
			if (redraw) {
				consoleScreen(GFX_TOP);
				consoleClear();
				consolePrintHeader();
				std::printf("\n  %sRestore failed%s.\n\n  " \
					"Something went wrong while trying to restore," \
					"\n  see screen below for details.\n\n  " \
					"Reason for failure: %s\n\n  "
					"If you think this is a bug, please open an\n  " \
					"issue on the following URL:\n  https://github.com/Hamcha/lumaupdate/issues\n\n  " \
					"Press START to exit.\n",
					CONSOLE_RED, CONSOLE_RESET, result.errcode.c_str());
				redraw = false;
			}
			break;
		}

		if ((kDown & KEY_START) != 0) {
			// Exit
			break;
		}

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

cleanup:
	// Exit services
	httpcExit();
	gfxExit();
	amExit();
	aptExit();
	return 0;
}
