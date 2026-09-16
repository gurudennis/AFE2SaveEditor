#include <AFE2Save.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>
#include <fstream>

namespace AFE2S {

namespace SaveFileUtils {

namespace {

static constexpr std::byte xorKey{ 0x42 };

} // anonymous namespace

std::vector<std::byte> readSaveFile(const std::filesystem::path& path) {
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw new std::runtime_error(std::format("Failed to open \"{}\"", path.string()));
    }

    size_t fileSize = std::filesystem::file_size(path);
    if (fileSize == 0) {
        return {};
    }

    std::vector<std::byte> buffer;
    buffer.resize(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    return buffer;
}

void writeSaveFile(const std::filesystem::path& path, std::span<const std::byte> data) {
    std::ofstream file(path.c_str(), std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        throw new std::runtime_error(std::format("Failed to open \"{}\"", path.string()));
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

std::vector<std::byte> encryptSave(std::string_view data) {
    if (data.empty()) {
        return {};
    }

    std::vector<std::byte> res;
    res.reserve(data.size());
    std::transform(data.cbegin(), data.cend() - 1, std::back_inserter(res), [](char c) { return std::byte(c) ^ xorKey; });

    res.emplace_back(std::byte(data.back())); // obfuscation: the last byte isn't XOR'd

    return res;
}

std::string decryptSave(std::span<const std::byte> data) {
    if (data.empty()) {
        return {};
    }

    std::string res;
    res.reserve(data.size());
    std::transform(data.begin(), data.end() - 1, std::back_inserter(res), [](std::byte c) { return char(c ^ xorKey); });

    res += char(data.back()); // obfuscation: the last byte isn't XOR'd

    return res;
}

} // namespace SaveFileUtils

namespace Impl {

namespace {

static constexpr size_t jsonIndent = 4;

} // anonymous namespace

class SaveStateImpl {
public:
    explicit SaveStateImpl(std::string_view jsonStr)
        : json_(nlohmann::json::parse(jsonStr)) {
    }

    std::string getJSON() const {
        return json_.dump(jsonIndent);
    }

    // ...

private:
    nlohmann::json json_;
};

} // namespace Impl

//
// SaveState
//

SaveState::SaveState(std::string_view json)
    : impl_(std::make_unique<Impl::SaveStateImpl>(json)) {
}

SaveState::~SaveState() = default;

std::string SaveState::getJSON() const {
    return impl_->getJSON();
}

//
// Free functions
//

std::filesystem::path getDefaultSaveFilePath() {
    // ...
    return {};
}

SaveState readSaveFile(const std::filesystem::path& path) {
    return SaveState(SaveFileUtils::decryptSave(SaveFileUtils::readSaveFile(path)));
}

void writeSaveFile(const std::filesystem::path& path, const SaveState& save) {
    SaveFileUtils::writeSaveFile(path, SaveFileUtils::encryptSave(save.getJSON()));
}

} // namespace AFE2S
