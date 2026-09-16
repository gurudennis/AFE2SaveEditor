#include <filesystem>
#include <iostream>
#include <conio.h>

#include <cxxopts.hpp>

#include <AFE2Save.hpp>

#include "Version.hpp"

void interactiveEdit(AFE2S::SaveState& save, const std::filesystem::path& path, cxxopts::ParseResult& cmdLine) {
    // ...
}

int main(int argc, const char** argv) {
    constexpr const char* appName = "AFE2SaveEditor v." AFE2SAVEEDITOR_VERSION;
    constexpr const char* appDescription = "Save editor for Aliens Fireteam Elite 2 (the game)";
    std::cout << appName << std::endl << appDescription << std::endl;

    try {
        cxxopts::Options options("");
        options.add_options()
            ("h,help", "Display this help message")
            ("f,file", "Path to the save file (auto-detected if not specified)", cxxopts::value<std::string>());

        cxxopts::ParseResult cmdLine = options.parse(argc, argv);
        if (cmdLine.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        std::cout << std::endl;

        std::filesystem::path filePath;
        if (cmdLine.count("file")) {
            filePath = cmdLine["file"].as<std::string>();
        } else {
            filePath = AFE2S::getDefaultSaveFilePath();
        }

        std::cout << "Save file: \"" << filePath.string() << "\"" << std::endl << std::endl;

        AFE2S::SaveState save = AFE2S::readSaveFile(filePath);
        interactiveEdit(save, filePath, cmdLine);
    } catch (const std::exception& ex) {
        std::cout << "Error! " << ex.what() << std::endl;
        return 1;
    }

    std::cout << "Press any key to close... ";
    std::cout.flush();
    std::ignore = _getch();

    return 0;
}
