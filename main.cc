#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

void log_error(std::ofstream& log_file, const std::string& message) {
    log_file << "Error: " << message << std::endl;
}

// Returns std::nullopt only if the top-level directory can't be opened.
// Errors further down are logged and the scan keeps whatever it has found.
std::optional<std::vector<fs::path>> find_large_files(
    const fs::path& directory, const std::uintmax_t size_limit,
    std::ofstream& log_file) {
    std::vector<fs::path> large_files;
    std::error_code ec;
    fs::recursive_directory_iterator it(
        directory, fs::directory_options::skip_permission_denied, ec);
    if (ec) {
        const std::string message =
            "Cannot open directory " + directory.string() + ": " + ec.message();
        log_error(log_file, message);
        std::cerr << "Error: " << message << std::endl;
        return std::nullopt;
    }

    const fs::recursive_directory_iterator end;
    while (it != end) {
        const fs::directory_entry& entry = *it;
        // Skip symlinks so a linked file isn't reported twice.
        if (!entry.is_symlink(ec) && !ec && entry.is_regular_file(ec) && !ec) {
            const std::uintmax_t size = entry.file_size(ec);
            if (!ec && size > size_limit) {
                large_files.push_back(entry.path());
            }
        }
        if (ec) {
            log_error(log_file, "Cannot read " + entry.path().string() + ": " +
                                    ec.message());
            ec.clear();
        }

        // A failed increment leaves the iterator at end, so the scan stops
        // here but keeps the results found so far.
        const fs::path last = entry.path();
        it.increment(ec);
        if (ec) {
            log_error(log_file, "Scan stopped after " + last.string() + ": " +
                                    ec.message());
            break;
        }
    }
    return large_files;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <directory> <log_file_path>"
                  << std::endl;
        return 1;
    }

    const fs::path directory = argv[1];
    const fs::path log_file_path = argv[2];
    constexpr std::uintmax_t size_limit =
        4ULL * 1024 * 1024 * 1024;  // 4 GB in bytes

    std::ofstream log_file(log_file_path, std::ios::out | std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Unable to open log file for writing: " << log_file_path
                  << std::endl;
        return 1;
    }

    auto large_files_opt = find_large_files(directory, size_limit, log_file);
    if (!large_files_opt) {
        return 1;
    }

    const auto& large_files = *large_files_opt;
    std::cout << "Files larger than 4 GB:" << std::endl;
    for (const auto& file : large_files) {
        // Print the plain string; operator<< on a path adds quotes.
        std::cout << file.string() << std::endl;
    }

    return 0;
}
