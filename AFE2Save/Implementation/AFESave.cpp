#include <windows.h>
#include <shlobj.h>

#include <AFE2Save.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <iostream>

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

std::vector<std::byte> encryptSave(std::string_view data, bool no_op) {
    if (data.empty()) {
        return {};
    }

    std::vector<std::byte> res;
    res.reserve(data.size());
    std::transform(data.cbegin(), data.cend() - 1, std::back_inserter(res), [no_op](char c) {
        return no_op ? std::byte(c) : std::byte(c) ^ xorKey;
    });

    res.emplace_back(std::byte(data.back())); // obfuscation: the last byte isn't XOR'd

    return res;
}

std::string decryptSave(std::span<const std::byte> data, bool no_op) {
    if (data.empty()) {
        return {};
    }

    std::string res;
    res.reserve(data.size());
    std::transform(data.begin(), data.end() - 1, std::back_inserter(res), [no_op](std::byte c) {
        return char(no_op ? c : c ^ xorKey);
    });

    res += char(data.back()); // obfuscation: the last byte isn't XOR'd

    return res;
}

} // namespace SaveFileUtils

namespace Impl {

namespace {

static constexpr size_t jsonIndentSize = 1;
static constexpr char jsonIndentChar = '\t';

} // anonymous namespace

class SaveStateImpl {
public:
    explicit SaveStateImpl(std::string_view jsonStr)
        : json_(nlohmann::json::parse(jsonStr)) {
    }

    bool isDirty() const {
        return isDirty_;
    }

    void resetDirty() const {
        isDirty_ = false;
    }

    std::string getJSON() const {
        return json_.dump(jsonIndentSize, jsonIndentChar);
    }

    SaveState::Info getInfo() const {
        SaveState::Info info{};
        info.accountID = json_["AccountId"].get<std::string>();
        info.categoryStats.rewardPackCount = uint32_t(getRewardPacks(json_).size());
        info.categoryStats.gunCount = uint32_t(json_["GunInventory"]["GunFrames"].size());
        info.categoryStats.gunModCount = uint32_t(getGunMods(json_).size());
        info.categoryStats.cosmeticCount = uint32_t(getCosmetics(json_).size());
        return info;
    }

    SaveState::CategoryStats importFrom(const Impl::SaveStateImpl& templ) {
        SaveState::CategoryStats stats{};
        stats.rewardPackCount = importSection(getRewardPacks(json_), getRewardPacks(templ.json_), getRewardPackKeyExtractor());
        stats.gunModCount = importSection(getGunMods(json_), getGunMods(templ.json_), getGunModKeyExtractor());
        stats.cosmeticCount = importSection(getCosmetics(json_), getCosmetics(templ.json_), getCosmeticKeyExtractor());
        isDirty_ = true;
        return stats;
    }

private:
    static auto& getRewardPacks(auto& json) {
        return json["RewardPackInventory"]["RewardPacks"];
    }

    static std::string getRewardPackKey(const nlohmann::json& item) {
        return item["RewardPackClass"].get<std::string>();
    }

    static auto getRewardPackKeyExtractor() {
        return [](const nlohmann::json& item) { return getRewardPackKey(item); };
    }

    static auto& getGunMods(auto& json) {
        return json["ModInventory"]["UnlimitedModStorage"];
    }

    static std::string getGunModKey(const nlohmann::json& item) {
        return item["ModDef"].get<std::string>();
    }

    static auto getGunModKeyExtractor() {
        return [](const nlohmann::json& item) { return getGunModKey(item); };
    }

    static auto& getCosmetics(auto& json) {
        return json["GeneralInventory"]["Items"];
    }

    static std::string getCosmeticKey(const nlohmann::json& item) {
        return item["Class"].get<std::string>();
    }

    static auto getCosmeticKeyExtractor() {
        return [](const nlohmann::json& item) { return getCosmeticKey(item); };
    }

    template <typename KeyExtractor>
    static uint32_t importSection(nlohmann::json& target, const nlohmann::json& source, KeyExtractor&& keyExtractor) {
        uint32_t addedCount = 0;
        for (const auto& item : source) {
            auto key = keyExtractor(item);
            if constexpr (std::is_same_v<decltype(key), std::string>) {
                if (key.empty()) {
                    continue;
                }
            }
            auto it = std::find_if(target.begin(), target.end(), [&key, &keyExtractor](const nlohmann::json& existingItem) {
                return keyExtractor(existingItem) == key;
            });
            if (it == target.end()) {
                ++addedCount;
#ifdef _DEBUG
                std::cout << "Imported: " << key << std::endl;
#endif
                target.emplace_back(item);
            }
        }
        return addedCount;
    }

    template <typename KeyExtractor>
    static uint32_t countDuplicates(const nlohmann::json& json, KeyExtractor&& keyExtractor) {
        uint32_t count = 0;
        std::set<std::string> seenKeys;
        for (const auto& item : json) {
            auto key = keyExtractor(item);
            if (seenKeys.find(key) != seenKeys.end()) {
                ++count;
#ifdef _DEBUG
                std::cout << "Duplicate: " << key << std::endl;
#endif
            } else {
                seenKeys.insert(key);
            }
        }
        return count;
    }

private:
    mutable bool isDirty_{};
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
SaveState::SaveState(SaveState&&) noexcept = default;
SaveState& SaveState::operator=(SaveState&&) noexcept = default;

bool SaveState::isDirty() const {
    return impl_->isDirty();
}

void SaveState::resetDirty() const {
    impl_->resetDirty();
}

std::string SaveState::getJSON() const {
    return impl_->getJSON();
}

SaveState::Info SaveState::getInfo() const {
    return impl_->getInfo();
}

AFE2S::SaveState::CategoryStats SaveState::importFrom(const SaveState& templ) {
    return impl_->importFrom(*templ.impl_);
}

//
// Free functions: backup management
//

namespace {

std::filesystem::path getBackupDirectory() {
    std::filesystem::path path{};

    wchar_t* rawPath{};
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &rawPath);
    if (SUCCEEDED(hr) && rawPath && *rawPath) {
        path = std::filesystem::path(rawPath);
    }
    if (rawPath != nullptr) {
        CoTaskMemFree(rawPath);
    }

    path = path / "AFE2Save" / "Backups";
    std::filesystem::create_directories(path);

    return path;
}

static constexpr const char* oldBackupName = "old.char.sav";
static constexpr const char* newBackupName = "new.char.sav";

std::filesystem::path getBackupFilePath(BackupType type, bool allowNonexistent = false, bool findSubstitute = true) {
    std::filesystem::path file;

    std::filesystem::path root = getBackupDirectory();
    switch (type) {
    case BackupType::Oldest:
        file = root / oldBackupName;
        break;
    case BackupType::Newest:
        file = root / newBackupName;
        break;
    default:
        throw std::invalid_argument("Invalid BackupType");
    }

    if (std::filesystem::exists(file) || allowNonexistent) {
        return file;
    }

    if (!findSubstitute) {
        throw std::invalid_argument("Backup not found");
    }

    return type == BackupType::Oldest ?
        getBackupFilePath(BackupType::Newest, false, false) :
        getBackupFilePath(BackupType::Oldest, false, false);
}

void makeBackup(const std::filesystem::path& path) {
    std::filesystem::path backupPath = getBackupFilePath(isBackupAvailable() ? BackupType::Newest : BackupType::Oldest, true, false);
    std::filesystem::copy_file(path, backupPath, std::filesystem::copy_options::overwrite_existing);
}

} // anonymous namespace

bool isBackupAvailable() {
    try {
        std::ignore = getBackupFilePath(BackupType::Newest);
        return true;
    } catch (const std::exception&) {
    }
    return false;
}

void restoreBackup(BackupType type, const std::filesystem::path& path) {
    std::filesystem::path backupPath = getBackupFilePath(type);
    std::filesystem::copy_file(backupPath, path, std::filesystem::copy_options::overwrite_existing);
}

//
// Free functions: save management
//

std::filesystem::path getDefaultSaveFilePath() {
    // ...
    return {};
}

SaveState readSaveFile(const std::filesystem::path& path) {
    std::string jsonStr = SaveFileUtils::decryptSave(SaveFileUtils::readSaveFile(path), path.extension() == ".json");
#ifdef _DEBUG
    if (path.extension() == ".sav") {
        SaveFileUtils::writeSaveFile(path.string() + ".json", SaveFileUtils::encryptSave(jsonStr, true));
    }
#endif

    return SaveState(jsonStr);
}

void writeSaveFile(const std::filesystem::path& path, const SaveState& save) {
    makeBackup(path);
    SaveFileUtils::writeSaveFile(path, SaveFileUtils::encryptSave(save.getJSON(), path.extension() == ".json"));
    save.resetDirty();
}

} // namespace AFE2S
