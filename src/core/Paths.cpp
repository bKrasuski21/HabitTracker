#include "core/Paths.hpp"

#include <cstdlib>
#include <system_error>

#if defined(_WIN32)
#include <windows.h>
#include <vector>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <vector>
#endif

namespace ht::paths {
namespace {

[[nodiscard]] std::filesystem::path executablePath() {
#if defined(_WIN32)
    std::vector<wchar_t> buffer(MAX_PATH);
    while (true) {
        const DWORD length =
            ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size()) {
            return std::filesystem::path(std::wstring(buffer.data(), length));
        }
        buffer.resize(buffer.size() * 2);
    }
#elif defined(__APPLE__)
    std::uint32_t size = 0;
    ::_NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (::_NSGetExecutablePath(buffer.data(), &size) != 0) {
        return {};
    }
    std::error_code ec;
    const std::filesystem::path resolved =
        std::filesystem::canonical(std::filesystem::path(buffer.data()), ec);
    return ec ? std::filesystem::path(buffer.data()) : resolved;
#else
    std::error_code ec;
    const std::filesystem::path resolved =
        std::filesystem::read_symlink("/proc/self/exe", ec);
    return ec ? std::filesystem::path{} : resolved;
#endif
}

}  // namespace

std::filesystem::path executableDirectory() {
    const std::filesystem::path exe = executablePath();
    if (exe.empty()) {
        return {};
    }
    return exe.parent_path();
}

std::filesystem::path locateResource(const std::filesystem::path& fileName) {
    std::error_code ec;
    const std::filesystem::path beside = executableDirectory() / fileName;
    if (!executableDirectory().empty() && std::filesystem::exists(beside, ec)) {
        return beside;
    }
    if (std::filesystem::exists(fileName, ec)) {
        return fileName;
    }
    return {};
}

std::filesystem::path defaultDataDirectory() {
    if (const char* const override = std::getenv("HABITTRACKER_DATA_DIR");
        override != nullptr && *override != '\0') {
        return std::filesystem::path(override);
    }

    // A release ships data/ beside the binary; a development build runs the
    // binary out of build/ with data/ in the working directory. Prefer whichever
    // already exists, and fall back to beside the binary when neither does.
    std::error_code ec;
    const std::filesystem::path exeDir = executableDirectory();
    const std::filesystem::path beside = exeDir / "data";
    if (!exeDir.empty() && std::filesystem::is_directory(beside, ec)) {
        return beside;
    }
    const std::filesystem::path here("data");
    if (std::filesystem::is_directory(here, ec)) {
        return here;
    }
    return exeDir.empty() ? here : beside;
}

}  // namespace ht::paths
