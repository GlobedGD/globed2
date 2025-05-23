#pragma once

#include <optional>
#include <string>
#include <filesystem>

namespace globed {
    bool hasBrokenResources();
    bool hasSeverelyBrokenResources();
    bool useFallbackMenuButton();
    void resetIntegrityCheck();

    void checkResources();

    enum class SheetIntegrity {
        Ok,
        MissingPng,
        MissingPlist,
    };

    struct IntegrityReport {
        SheetIntegrity sheetIntegrity;       // whether globedsheet1.png and globedsheet1.plist were found
        bool sheetFilesSeparated;            // whether the files are in different directories
        bool sheetFilesModified;             // whether the files are not in the mod resources directory
        bool darkmodeEnabled;                // whether DarkMode v4 is enabled
        bool hasTexturePack;                 // true if SheetIntegrity is not Ok, or either `sheetFilesSeparated` or `sheetFilesModified` is true
        std::optional<std::filesystem::path>
                impostorFolder;              // whether the impostor folder was found

        // following values only make sense if sheet ingerity is Ok
        bool menuIconPngFound;               // whether menuicon.png was found
        bool dummmyPngFound;                 // whether the dummy icon png was found

        std::string asDebugData();
    };

    IntegrityReport& getIntegrityReport();
    IntegrityReport createIntegrityReport();

    const std::filesystem::path& getLatestLogFile();
}
