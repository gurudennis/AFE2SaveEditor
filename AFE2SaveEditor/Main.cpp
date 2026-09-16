#include <filesystem>
#include <iostream>

#include <cxxopts.hpp>

#include <AFE2Save.hpp>

#include "Version.hpp"

int main(int argc, const char** argv) {
    constexpr const char* appName = "AFE2SaveEditor v." AFE2SAVEEDITOR_VERSION;
    constexpr const char* appDescription = "Save editor for Aliens Fireteam Elite 2 (the game)";
    std::cout << appName << std::endl << appDescription << std::endl;

    cxxopts::Options options("");
    options.add_options()
        ("h,help", "Display this help message")
        ("f,file", "Path to the save file (auto-detected if not specified)", cxxopts::value<std::string>());

    cxxopts::ParseResult result = options.parse(argc, argv);
    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    std::cout << std::endl;

    std::filesystem::path filePath;
    if (result.count("file")) {
        filePath = result["file"].as<std::string>();
    } else {
        filePath = AFE2S::getDefaultSaveFilePath();
    }

    std::cout << "Save file: \"" << filePath.string() << "\"" << std::endl << std::endl;

    AFE2S::SaveState save = AFE2S::readSaveFile(filePath);

    // TODO: do something useful
    // ...

    return 0;
}
