#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

std::vector<fs::path> find_large_files(const fs::path& directory,
                                       uintmax_t size_limit,
                                       std::ofstream& log_file) {
    std::vector<fs::path> large_files;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(
                 directory, fs::directory_options::skip_permission_denied)) {
            try {
                if (fs::is_regular_file(entry) &&
                    fs::file_size(entry) > size_limit) {
                    large_files.push_back(entry.path());
                }
            } catch (const fs::filesystem_error& e) {
                log_file << "Error reading file: " << entry.path() << " - "
                         << e.what() << std::endl;
            }
        }
    } catch (const fs::filesystem_error& e) {
        log_file << "Error: " << e.what() << std::endl;
    }
    return large_files;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <directory> <log_file_path>"
                  << std::endl;
        return 1;
    }

    fs::path directory = argv[1];
    fs::path log_file_path = argv[2];
    uintmax_t size_limit = 4ULL * 1024 * 1024 * 1024;  // 4 GB in bytes

    std::ofstream log_file(log_file_path, std::ios::out | std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Unable to open log file for writing: " << log_file_path
                  << std::endl;
        return 1;
    }

    std::vector<fs::path> large_files =
        find_large_files(directory, size_limit, log_file);

    std::cout << "Files larger than 4 GB:" << std::endl;
    for (const auto& file : large_files) {
        std::cout << file << std::endl;
    }

    log_file.close();
    return 0;
}
