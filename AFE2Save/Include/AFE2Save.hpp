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

    std::string getJSON() const;

    // ...

private:
    std::unique_ptr<Impl::SaveStateImpl> impl_;
};

std::filesystem::path getDefaultSaveFilePath();
SaveState readSaveFile(const std::filesystem::path& path);
void writeSaveFile(const std::filesystem::path& path, const SaveState& save);

} // namespace AFE2S
