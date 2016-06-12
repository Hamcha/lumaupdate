#include "libs.h"

#include "autoupdate.h"
#include "arnutil.h"
#include "config.h"
#include "http.h"
#include "lumautils.h"
#include "console.h"
#include "pathchange.h"
#include "release.h"
#include "utils.h"
#include "version.h"

#define WAIT_START while (aptMainLoop() && !(hidKeysDown() & KEY_START)) { gspWaitForVBlank(); hidScanInput(); }

bool redraw = false;
Config config;
std::string errcode = "NO REASON";

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

struct UpdateArgs {
	// Detected options
	std::string  currentVersion = "";
	std::string  backupVersion  = "";
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
};

int drawChangelog(const std::string& name, const std::string& log, const int page) {
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
		printf("%sRelease notes for %sv%s%s\n\n", CONSOLE_YELLOW, CONSOLE_GREEN, name.c_str(), CONSOLE_RESET);

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

UpdateChoice drawConfirmationScreen(const UpdateArgs& args, const bool usingConfig) {
	static bool redrawTop = false;
	static bool redrawBottom = false;
	static bool partialredraw = false;
	static int  selected = 0;
	static int  hourlyOptionStart = INT_MAX;
	static int  extraOptionStart  = INT_MAX;
	static int  currentPage = 0;
	static int  pageCount = 0;

	const std::string latestStable = versionGetStable(args.currentVersion);
	const std::string latestCommit = versionGetCommit(args.currentVersion);

	const bool isDev = args.currentVersion.find("(dev)") != std::string::npos;

	const bool haveLatestStable = latestStable == args.stable->name;
	const bool haveLatestCommit = latestCommit == args.hourly->commits[isDev ? "dev hourly" : "hourly"];

	const bool backupVersionDetected = args.backupExists && args.backupVersion != "";

	const u32 keydown = hidKeysDown();

	if (keydown & KEY_UP) {
		partialredraw = true;
		--selected;
	}
	if (keydown & KEY_DOWN) {
		partialredraw = true;
		++selected;
	}
	if (keydown & KEY_L && currentPage > 0) {
		--currentPage;
		redrawBottom = true;
	}
	if (keydown & KEY_R && currentPage < pageCount - 1) {
		++currentPage;
		redrawBottom = true;
	}

	if (keydown & KEY_A) {
		if (selected < hourlyOptionStart) {
			return UpdateChoice(ChoiceType::UpdatePayload, args.stable->versions[selected], false);
		}
		if (selected < extraOptionStart) {
			return UpdateChoice(ChoiceType::UpdatePayload, args.hourly->versions[selected - hourlyOptionStart], true);
		}
		int extraOptionID = selected - extraOptionStart;

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
			selected = 0;
			return UpdateChoice(ChoiceType::NoChoice);
		}
	}

	if (redraw) {
		redrawTop = redrawBottom = true;
	}

	if (!redrawTop && !redrawBottom && !partialredraw) {
		return UpdateChoice(ChoiceType::NoChoice);
	}

	consoleScreen(GFX_TOP);

	if (redrawTop) {
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

		std::printf("\n  Choose action:\n");

		consolePrintFooter();
	}

	if (redrawBottom) {
		pageCount = drawChangelog(args.stable->name, args.stable->description, currentPage);
		consoleScreen(GFX_TOP);
	}

	int y = 11;

	if (!usingConfig) {
		y += 3;
	}
	if (args.hourly != nullptr) {
		y += 1;
	}
	if (backupVersionDetected) {
		y += 1;
	}

	consoleMoveTo(0, y);

	int optionCount = args.stable->versions.size() + (args.hourly != nullptr ? args.hourly->versions.size() : 0) + (args.backupExists ? 1 : 0);

	// Wrap around cursor
	while (selected < 0) selected += optionCount;
	selected = selected % optionCount;

	int curOption = 0;
	for (ReleaseVer r : args.stable->versions) {
		printf("     Install %s\n", r.friendlyName.c_str());
		++curOption;
	}

	hourlyOptionStart = curOption;
	if (args.hourly != nullptr) {
		for (ReleaseVer h : args.hourly->versions) {
			printf("     Install %s\n", h.friendlyName.c_str());
			++curOption;
		}
	}

	extraOptionStart = curOption;

	// Extra #0: Restore backup
	if (args.backupExists) {
		printf("     Restore backup\n");
		++curOption;
	}

	// Print cursor
	consoleMoveTo(3, y + selected);
	printf("\x10");

	// Reset redraw vars
	redraw = redrawTop = redrawBottom = partialredraw = false;
	return UpdateChoice(ChoiceType::NoChoice);
}

SelfUpdateChoice drawUpdateNag(const LatestUpdaterInfo& latest) {
	static bool redrawTop = false;
	static bool redrawBottom = false;
	static bool partialredraw = false;
	static int  selected = 0;
	static int  currentPage = 0;
	static int  pageCount = 0;

	const u32 keydown = hidKeysDown();

	if (keydown & KEY_UP) {
		--selected;
		partialredraw = true;
	}
	if (keydown & KEY_DOWN) {
		++selected;
		partialredraw = true;
	}
	if (keydown & KEY_L && currentPage > 0) {
		--currentPage;
		redrawBottom = true;
	}
	if (keydown & KEY_R && currentPage < pageCount - 1) {
		++currentPage;
		redrawBottom = true;
	}

	if (keydown & KEY_A) {
		switch (selected) {
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
			selected = 0;
			return SelfUpdateChoice::NoChoice;
		}
	}

	if (redraw) {
		redrawTop = redrawBottom = true;
	}

	if (!redrawTop && !redrawBottom && !partialredraw) {
		return SelfUpdateChoice::NoChoice;
	}

	if (redrawTop) {
		consoleScreen(GFX_TOP);
		consoleClear();

		consoleMoveTo(5, 10);
		std::printf("%sAn update for LumaUpdater is available!%s", CONSOLE_YELLOW, CONSOLE_RESET);

		consoleMoveTo(15, 12);
		std::printf("LumaUpdater %s%s%s", CONSOLE_GREEN, latest.version.c_str(), CONSOLE_RESET);
		consoleMoveTo(10, 13);
		std::printf("See screen below for changes", CONSOLE_GREEN, latest.version.c_str(), CONSOLE_RESET);

		consoleMoveTo(12, 15);
		std::printf("Would you like to update?");

		consolePrintFooter(true);
	}

	if (redrawBottom) {
		pageCount = drawChangelog(latest.version, latest.changelog, currentPage);
		consoleScreen(GFX_TOP);
	}

	selected = selected % 2;

	redraw = redrawTop = redrawBottom = partialredraw = false;
	return SelfUpdateChoice::NoChoice;
}

bool backupA9LH(const std::string& payloadName) {
	std::ifstream original(payloadName, std::ifstream::binary);
	if (!original.good()) {
		std::printf("Could not open %s\n", payloadName.c_str());
		return false;
	}

	std::string backupName = payloadName + ".bak";
	std::ofstream target(backupName, std::ofstream::binary);
	if (!target.good()) {
		std::printf("Could not open %s\n", backupName.c_str());
		original.close();
		return false;
	}

	target << original.rdbuf();

	original.close();
	target.close();
	return true;
}

bool update(const UpdateArgs& args) {
	consoleScreen(GFX_TOP);
	consoleInitProgress("Updating Luma3DS", "Performing preliminary operations", 0);

	consoleScreen(GFX_BOTTOM);
	consoleClear();

	// Back up local file if it exists
	if (!args.backupExisting) {
		std::printf("Payload backup is disabled in config, skipping...\n");
	} else if (!fileExists(args.payloadPath)) {
		std::printf("Original payload not found, skipping backup...\n");
	} else {
		consoleScreen(GFX_TOP);
		consoleSetProgressData("Backing up old payload", 0.1);
		consoleScreen(GFX_BOTTOM);

		std::printf("Copying %s to %s.bak...\n", args.payloadPath.c_str(), args.payloadPath.c_str());
		gfxFlushBuffers();
		if (!backupA9LH(args.payloadPath)) {
			std::printf("\nCould not backup %s (!!), aborting...\n", args.payloadPath.c_str());
			errcode = "BACKUP FAILED";
			return false;
		}
	}

	consoleScreen(GFX_TOP);
	consoleSetProgressData("Downloading payload", 0.3);
	consoleScreen(GFX_BOTTOM);

	std::printf("Downloading %s\n", args.choice.chosenVersion.url.c_str());
	gfxFlushBuffers();

	u8* payloadData = nullptr;
	size_t offset = 0;
	size_t payloadSize = 0;
	bool ret = releaseGetPayload(args.choice.chosenVersion, args.choice.isHourly, &payloadData, &offset, &payloadSize);
	if (!ret) {
		std::printf("FATAL\nCould not get A9LH payload...\n");
		std::free(payloadData);
		errcode = "DOWNLOAD FAILED";
		return false;
	}

	if (args.payloadPath != std::string("/") + PAYLOADPATH) {
		consoleScreen(GFX_TOP);
		consoleSetProgressData("Applying path changing", 0.6);
		consoleScreen(GFX_BOTTOM);

		std::printf("Requested payload path is not %s, applying path patch...\n", PAYLOADPATH);
		bool res = pathchange(payloadData + offset, payloadSize, args.payloadPath);
		if (!res) {
			std::free(payloadData);
			errcode = "PATHCHANGE FAILED";
			return false;
		}
	}

	if (args.migrateARN) {
		consoleScreen(GFX_TOP);
		consoleSetProgressData("Migrating AuReiNand -> Luma3DS", 0.8);
		consoleScreen(GFX_BOTTOM);

		std::printf("Migrating AuReiNand install to Luma3DS...\n");
		if (!arnMigrate()) {
			std::printf("FATAL\nCould not migrate AuReiNand install (?)\n");
			errcode = "MIGRATION FAILED";
			return false;
		}
	}

	if (!lumaMigratePayloads()) {
		std::printf("WARN\nCould not migrate payloads\n\n");
	}

	consoleScreen(GFX_TOP);
	consoleSetProgressData("Saving payload to SD", 0.9);
	consoleScreen(GFX_BOTTOM);

	std::printf("Saving %s to SD (as %s)...\n", PAYLOADPATH, args.payloadPath.c_str());
	std::ofstream a9lhfile("/" + args.payloadPath, std::ofstream::binary);
	a9lhfile.write((const char*)(payloadData + offset), payloadSize);
	a9lhfile.close();

	std::printf("All done, freeing resources and exiting...\n");
	std::free(payloadData);

	consoleClear();
	consoleScreen(GFX_TOP);

	return true;
}

bool restore(const UpdateArgs& args) {
	// Rename current payload to .broken
	if (std::rename(args.payloadPath.c_str(), (args.payloadPath + ".broken").c_str()) != 0) {
		std::perror("Can't rename current version");
		errcode = "RENAME 1 FAILED";
		return false;
	}
	// Rename .bak to current
	if (std::rename((args.payloadPath + ".bak").c_str(), args.payloadPath.c_str()) != 0) {
		std::perror("Can't rename backup to current payload name");
		errcode = "RENAME 2 FAILED";
		return false;
	}
	// Remove .broken
	if (std::remove((args.payloadPath + ".broken").c_str()) != 0) {
		std::perror("WARN: Could not remove current payload, please remove it manually");
	}
	return true;
}

int main(int argc, char* argv[]) {
	std::vector<std::string> cfgPaths = {
		"/lumaupdater.cfg",
		"/3DS/lumaupdater.cfg",
		"/luma/lumaupdater.cfg",
	};

	UpdateState state = UpdateConfirmationScreen;
	ReleaseInfo release = {}, hourly = {};
	UpdateArgs updateArgs = {};
	bool nohourly = false; // Used if fetching hourlies fails

	aptInit();
	gfxInitDefault();
	httpcInit(0);

	consoleInitEx();

	consoleScreen(GFX_TOP);
	consoleInitProgress("Loading Luma3DS Updater", "Reading configuration file", 0);
	consoleScreen(GFX_BOTTOM);

	// If argv0 is present, add its path (without file) to config paths
	if (argc > 0) {
		std::string path(argv[0]);
		size_t start = path.find_first_of(':') + 1;
		std::string realpath = path.substr(start, path.find_last_of('/') - start);
		cfgPaths.push_back(realpath + "/lumaupdater.cfg");
	}

	// Read config file
	bool configFound = false;
	for (const std::string& path : cfgPaths) {
		LoadConfigError confStatus = config.LoadFile(path);
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

	// Check required values in config, if existing
	if (configFound && !config.Has("payload path")) {
		std::printf("Missing required config value: payload path\n");
		gfxFlushBuffers();
		WAIT_START
		goto cleanup;
	}
	
	if (!configFound) {
		std::printf("The configuration file could not be found, skipping...\n");
	}

	// Load config values
	updateArgs.payloadPath = config.Get("payload path", PAYLOADPATH);
	updateArgs.backupExisting = tolower(config.Get("backup", "y")[0]) == 'y';
	updateArgs.selfUpdate = tolower(config.Get("selfupdate", "y")[0]) == 'y';

	// Add initial slash to payload path, if missing
	if (updateArgs.payloadPath[0] != '/') {
		updateArgs.payloadPath = "/" + updateArgs.payloadPath;
	}

	// Check that the payload path is valid
	if (updateArgs.payloadPath.length() > MAXPATHLEN) {
		std::printf("\nFATAL\nPayload path is too long!\nIt can contain at most %d characters!\n\nPress START to quit.\n", MAXPATHLEN);
		gfxFlushBuffers();
		WAIT_START
		goto cleanup;
	}

	if (updateArgs.selfUpdate) {
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
		/*
		if (info.location == HomebrewLocation::Remote) {
			std::printf("Updater launched over 3DSLink, skipping self-update...\n");
			selfupdateContinue = false;
		}
		*/

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
	updateArgs.currentVersion = versionMemsearch(updateArgs.payloadPath);

	// Detect bak version, if exists
	if (fileExists(updateArgs.payloadPath + ".bak")) {
		updateArgs.backupExists = true;
		updateArgs.backupVersion = versionMemsearch(updateArgs.payloadPath + ".bak");
	}

	// Check for eventual migration from ARN to Luma
	updateArgs.migrateARN = arnVersionCheck(updateArgs.currentVersion);

	consoleScreen(GFX_TOP);
	consoleSetProgressData("Fetching latest release data", 0.6);
	consoleScreen(GFX_BOTTOM);

	try {
		release = releaseGetLatestStable();
	} catch (const std::string& e) {
		std::printf("%s\n", e.c_str());
		std::printf("\nFATAL ERROR\nFailed to obtain required data.\n\nPress START to exit.\n");
		gfxFlushBuffers();
		WAIT_START
		goto cleanup;
	}

	updateArgs.stable = &release;

	consoleScreen(GFX_TOP);
	consoleSetProgressData("Fetching latest hourly", 0.8);
	consoleScreen(GFX_BOTTOM);

	try {
		hourly = releaseGetLatestHourly();
	} catch (const std::string& e) {
		std::printf("%s\n", e.c_str());
		std::printf("\nWARN\nCould not obtain latest hourly, skipping...\n");
		nohourly = true;
		gfxFlushBuffers();
	}

	if (!nohourly) {
		updateArgs.hourly = &hourly;
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
			updateArgs.choice = drawConfirmationScreen(updateArgs, configFound);
			switch (updateArgs.choice.type) {
			case ChoiceType::UpdatePayload:
				state = Updating;
				redraw = true;
				break;
			case ChoiceType::RestoreBackup:
				state = Restoring;
				redraw = true;
				break;
			default:
				break;
			}
			break;
		case Updating:
			if (update(updateArgs)) {
				state = UpdateComplete;
			} else {
				state = UpdateFailed;
			}
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
					"Press START to exit.\n", CONSOLE_RED, CONSOLE_RESET, errcode.c_str());
				redraw = false;
			}
			break;
		case UpdateComplete:
			if (redraw) {
				consoleClear();
				consolePrintHeader();
				std::printf("\n  %sUpdate complete.%s\n", CONSOLE_GREEN, CONSOLE_RESET);
				if (updateArgs.backupExisting) {
					std::printf("\n  In case something goes wrong you can restore\n  the old payload from %s.bak\n", updateArgs.payloadPath.c_str());
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
			if (restore(updateArgs)) {
				state = RestoreComplete;
			} else {
				state = RestoreFailed;
			}
			redraw = true;
			break;
		case RestoreComplete:
			if (redraw) {
				consoleClear();
				consolePrintHeader();
				std::printf("\n  %sRestore complete.%s\n", CONSOLE_GREEN, CONSOLE_RESET);
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
					"Press START to exit.\n", CONSOLE_RED, CONSOLE_RESET, errcode.c_str());
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
	aptExit();
	return 0;
}
