#pragma once

#include <memory>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace AFE2S {

namespace SaveFileUtils {
std::vector<std::byte> readSaveFile(const std::filesystem::path& path);
void writeSaveFile(const std::filesystem::path& path, std::span<const std::byte> data);
std::vector<std::byte> encryptSave(std::string_view data, bool no_op = false);
std::string decryptSave(std::span<const std::byte> data, bool no_op = false);
} // namespace SaveFileUtils

namespace Impl {
class SaveStateImpl;
} // namespace Impl

class SaveState {
public:
    explicit SaveState(std::string_view json);
    ~SaveState();

    SaveState(SaveState&&) noexcept;
    SaveState& operator=(SaveState&&) noexcept;

    bool isDirty() const;
    void resetDirty() const;

    std::string getJSON() const;

    struct CategoryStats {
        uint32_t rewardPackCount{};
        uint32_t gunCount{};
        uint32_t gunModCount{};
        uint32_t cosmeticCount{};
    };

    struct Info {
        std::string accountID{};
        CategoryStats categoryStats{};
    };
    Info getInfo() const;

    CategoryStats importFrom(const SaveState& templ);

private:
    std::unique_ptr<Impl::SaveStateImpl> impl_;
};

std::filesystem::path getDefaultSaveFilePath();
SaveState readSaveFile(const std::filesystem::path& path);
void writeSaveFile(const std::filesystem::path& path, const SaveState& save);

} // namespace AFE2S
