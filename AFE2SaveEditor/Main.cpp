#include <windows.h>

#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <vector>
#include <conio.h>

#include <cxxopts.hpp>

#include <AFE2Save.hpp>

#include "Version.hpp"
#include "resource.h"

class App {
public:
    App(cxxopts::ParseResult cmdLine)
        : cmdLine_(cmdLine) {
        setupCommandPrototypes();
    }

    void Run() {
        std::string path = cmdLine_.count("file") ? cmdLine_["file"].as<std::string>() : std::string{};
        load(path);

        if (cmdLine_.count("advanced")) {
            interactiveAdvanced();
        } else {
            interactiveSimple();
        }
    }

private:
    struct Command {
        std::string name{};
        std::vector<std::string> args{};

        template <typename T>
        T getArg(uint8_t index, const char* descr) const {
            std::string str;
            if (index >= args.size()) {
                std::cout << "Enter " << descr << ": ";
                std::getline(std::cin, str);
            } else {
                str = args[index];
            }
            T value{};
            std::istringstream iss(str);
            iss >> value;
            if (!value.empty() && value.front() == '\"' && value.back() == '\"') { // quotes not supported, and simply ignored
                value.pop_back();
                value.erase(value.begin());
            }
            return value;
        }
    };

    Command getCommand() {
        std::cout << "Command (h for help): ";
        std::string line;
        std::getline(std::cin, line);
        Command cmd;
        std::istringstream iss(line);
        iss >> cmd.name;
        std::string arg;
        while (iss >> arg) {
            cmd.args.push_back(arg);
        }
        return cmd;
    }

    struct CommandPrototype {
        std::string shortName{};
        std::string longName{};
        std::string description{};
        uint8_t maxArgs{};
        std::function<bool(const Command&)> handler{};
    };

    void interactiveSimple() {
        bool exit = false;
        while (true) {
            std::cout << std::endl
                      << "Options:" << std::endl
                      << "1. Show save summary" << std::endl
                      << "2. Unlock everything" << std::endl
                      << "3. Restore recent backup" << std::endl
                      << "4. Restore oldest backup" << std::endl
                      << "5. Advanced mode" << std::endl
                      << "6. Exit" << std::endl
                      << std::endl
                      << "Press a number key to select an option... ";
            std::cout.flush();
            char c = _getch();
            std::cout << c << std::endl << std::endl;
            switch (c) {
            case '1':
                showSaveSummary();
                break;
            case '2':
                unlockEverything();
                save();
                break;
            case '3':
                restoreBackup(Command{ "restore_bak", {"newest"} });
                break;
            case '4':
                restoreBackup(Command{ "restore_bak", {"oldest"} });
                break;
            case '5':
                if (!interactiveAdvanced()) {
                    exit = true;
                }
                break;
            case '6':
                exit = true;
                break;
            }

            if (exit) {
                break;
            }
        }
    }

    void setupCommandPrototypes() {
        CommandPrototype commandPrototypes[] = {
            {"h",  "help",        "Show available commands",  0, [this](const Command& cmd) { showAdvancedHelp(); return true; }},
            {"m",  "summary",     "Show save summary",        0, [this](const Command& cmd) { showSaveSummary(); return true; }},
            {"u",  "unlock_all",  "Unlock everything",        0, [this](const Command& cmd) { unlockEverything(); return true; }},
            {"i",  "import_all",  "Import everything",        0, [this](const Command& cmd) { importEverything(cmd); return true; }},
            {"l",  "load",        "Load a save",              0, [this](const Command& cmd) { load(cmd); return true; }},
            {"s",  "save",        "Save the changes",         0, [this](const Command& cmd) { save(cmd); return true; }},
            {"r",  "restore_bak", "Restore a backup",         0, [this](const Command& cmd) { restoreBackup(cmd); return true; }},
            {"x",  "exit",        "Exit the app",             0, [this](const Command& cmd) { return exit(); }},
        };
        commandPrototypes_.reserve(std::size(commandPrototypes));
        for (auto& cmd : commandPrototypes) {
            commandPrototypes_.emplace_back(std::move(cmd));
        }
    }

    bool interactiveAdvanced() {
        while (true) {
            std::cout << std::endl;
            Command command = getCommand();
            bool found = false;
            for (const auto& cmdProto : commandPrototypes_) {
                if (cmdProto.shortName == command.name || cmdProto.longName == command.name) {
                    found = true;
                    try {
                        if (!cmdProto.handler(command)) {
                            return false;
                        }
                    } catch (const std::exception& ex) {
                        std::cout << "Error! " << ex.what() << std::endl;
                    }
                    break;
                }
            }
            if (!found) {
                std::cout << "Unknown command: " << command.name << std::endl;
            }
        }
        return true;
    }

    void showAdvancedHelp() {
        std::cout << "Available commands:" << std::endl;
        for (const auto& cmdProto : commandPrototypes_) {
            std::cout << "  " << cmdProto.shortName << ", " << cmdProto.longName << " - " << cmdProto.description << std::endl;
        }
    }

    void showSaveSummary() {
        AFE2S::SaveState::Info info = save_->getInfo();
        std::cout << "Save summary:" << std::endl
                  << "Account ID: " << info.accountID << std::endl
                  << "Reward packs: " << info.categoryStats.rewardPackCount << std::endl
                  << "Guns: " << info.categoryStats.gunCount << std::endl
                  << "Gun mods: " << info.categoryStats.gunModCount << std::endl
                  << "Cosmetics: " << info.categoryStats.cosmeticCount << std::endl;
    }

    void unlockEverything() {
        AFE2S::SaveState::CategoryStats stats = save_->importFrom(AFE2S::SaveState(loadTemplate()));
        std::cout << "Unlocked all known content:" << std::endl
                  << "  New reward packs: " << stats.rewardPackCount << std::endl
                  << "  New gun mods: " << stats.gunModCount << std::endl
                  << "  New cosmetics: " << stats.cosmeticCount << std::endl;
    }

    void importEverything(const std::string& templPath) {
        AFE2S::SaveState::CategoryStats stats = save_->importFrom(AFE2S::readSaveFile(templPath));
        std::cout << "Imported everything from \"" << templPath << "\":" << std::endl
                  << "  New reward packs: " << stats.rewardPackCount << std::endl
                  << "  New gun mods: " << stats.gunModCount << std::endl
                  << "  New cosmetics: " << stats.cosmeticCount << std::endl;
    }

    void importEverything(const Command& cmd) {
        std::string path = cmd.getArg<std::string>(0, "template save path");
        importEverything(path);
    }

    void load(const std::filesystem::path& path = {}) {
        std::filesystem::path loadPath = path;
        if (loadPath.empty()) {
            loadPath = AFE2S::getDefaultSaveFilePath();
        }

        save_.reset();

        std::cout << "Opening save file: \"" << loadPath.string() << "\"" << std::endl << std::endl;
        save_.emplace(AFE2S::readSaveFile(loadPath));
        savePath_ = loadPath;
    }

    void load(const Command& cmd) {
        if (!warnDirty()) {
            return;
        }
        std::string path = cmd.getArg<std::string>(0, "path (empty for default)");
        load(path);
    }

    void save(const std::string& path = {}) {
        std::string savePath = path;
        if (savePath.empty()) {
            savePath = savePath_.string();
        }
        AFE2S::writeSaveFile(savePath, *save_);
        std::cout << "Changes saved to \"" << savePath << "\"" << std::endl;
    }

    void save(const Command& cmd) {
        std::string path = cmd.getArg<std::string>(0, "path (empty to overwrite; [...].json to save as JSON)");
        save(path);
    }

    void restoreBackup(const Command& cmd) {
        if (!warnDirty()) {
            return;
        }

        std::cout << "You will LOSE ALL PROGRESS since the backup was taken. Are you absolutely sure? (y/n): ";
        std::cout.flush();
        char c = _getch();
        std::cout << c << std::endl;
        if (c != 'y' && c != 'Y') {
            return;
        }

        std::string typeStr = cmd.getArg<std::string>(0, "backup type (newest/oldest)");
        AFE2S::BackupType type{};
        if (typeStr == "newest") {
            type = AFE2S::BackupType::Newest;
        } else if (typeStr == "oldest") {
            type = AFE2S::BackupType::Oldest;
        } else {
            throw std::invalid_argument("Invalid backup type: " + typeStr);
        }
        AFE2S::restoreBackup(type, savePath_);
        std::cout << "Restored " << typeStr << " backup to \"" << savePath_ << "\"" << std::endl;
        
        load(savePath_);
    }

    bool exit() {
        if (!warnDirty()) {
            return true;
        }
        return false;
    }

    bool warnDirty() {
        if (save_ && save_->isDirty()) {
            std::cout << "Warning: unsaved changes exist. Do you want to continue? (y/n): ";
            std::cout.flush();
            char c = _getch();
            std::cout << c << std::endl;
            return c == 'y' || c == 'Y';
        }
        return true;
    }

    std::string loadTemplate() {
        HRSRC hRes = FindResource(nullptr, MAKEINTRESOURCE(IDR_RC_SAVE_TEMPLATE1), TEXT("RC_SAVE_TEMPLATE"));
        if (!hRes) {
            throw std::runtime_error("Failed to find template resource.");
        }

        HGLOBAL hData = LoadResource(nullptr, hRes);
        if (!hData) {
            throw std::runtime_error("Failed to load template resource.");
        }

        DWORD size = SizeofResource(nullptr, hRes);
        const char* data = reinterpret_cast<const char*>(LockResource(hData));
        if (!data) {
            throw std::runtime_error("Failed to lock template resource.");
        }

        return std::string(data, size);
    }

private:
    cxxopts::ParseResult cmdLine_{};
    std::filesystem::path savePath_{};
    std::optional<AFE2S::SaveState> save_{};
    std::vector<CommandPrototype> commandPrototypes_{};
};

int main(int argc, const char** argv) {
    constexpr const char* appName = "AFE2SaveEditor v." AFE2SAVEEDITOR_VERSION;
    constexpr const char* appDescription = "Save editor for Aliens Fireteam Elite 2 (the game)";
    std::cout << appName << std::endl
              << appDescription << std::endl
              << "USE AT YOUR OWN RISK!" << std::endl;

    try {
        cxxopts::Options options("");
        options.add_options()
            ("h,help", "Display this help message")
            ("f,file", "Path to the save file (auto-detected if not specified)", cxxopts::value<std::string>())
            ("a,advanced", "Start in advanced mode");

        cxxopts::ParseResult cmdLine = options.parse(argc, argv);
        if (cmdLine.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        std::cout << std::endl;

        App app(std::move(cmdLine));
        app.Run();

    } catch (const std::exception& ex) {
        std::cout << "Error! " << ex.what() << std::endl;
        return 1;
    }

    std::cout << "Press any key to close... ";
    std::cout.flush();
    std::ignore = _getch();

    return 0;
}
