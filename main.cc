#include <cctype>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

// Writes errors to the --log file with a timestamp, or to stderr if there is
// no log file.
class ErrorLog {
   public:
    bool open(const fs::path& path) {
        file_.open(path, std::ios::out | std::ios::app);
        return file_.is_open();
    }

    void error(const std::string& message) {
        if (file_.is_open()) {
            file_ << timestamp() << " Error: " << message << '\n';
        } else {
            std::cerr << "Error: " << message << '\n';
        }
    }

    // Fatal errors also go to stderr when logging to a file, so the user
    // sees why the program failed.
    void fatal(const std::string& message) {
        error(message);
        if (file_.is_open()) {
            std::cerr << "Error: " << message << '\n';
        }
    }

   private:
    static std::string timestamp() {
        const std::time_t now = std::time(nullptr);
        std::tm local{};
#ifdef _WIN32
        localtime_s(&local, &now);
#else
        localtime_r(&now, &local);
#endif
        std::ostringstream out;
        out << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
        return out.str();
    }

    std::ofstream file_;
};

// Parses a size such as "500", "64K" or "4G". Suffixes are powers of 1024
// and case-insensitive. Returns std::nullopt for anything else, including
// negative numbers and values that overflow.
std::optional<std::uintmax_t> parse_size(const std::string& text) {
    constexpr std::uintmax_t max = std::numeric_limits<std::uintmax_t>::max();

    std::size_t digits = 0;
    std::uintmax_t value = 0;
    while (digits < text.size() &&
           std::isdigit(static_cast<unsigned char>(text[digits]))) {
        const auto digit = static_cast<std::uintmax_t>(text[digits] - '0');
        if (value > (max - digit) / 10) {
            return std::nullopt;
        }
        value = value * 10 + digit;
        ++digits;
    }
    if (digits == 0 || text.size() - digits > 1) {
        return std::nullopt;
    }

    int shift = 0;
    if (digits < text.size()) {
        switch (std::toupper(static_cast<unsigned char>(text[digits]))) {
            case 'K': shift = 10; break;
            case 'M': shift = 20; break;
            case 'G': shift = 30; break;
            case 'T': shift = 40; break;
            default: return std::nullopt;
        }
    }
    if (value > (max >> shift)) {
        return std::nullopt;
    }
    return value << shift;
}

// Returns std::nullopt only if the top-level directory can't be opened.
// Errors further down are logged and the scan keeps whatever it has found.
std::optional<std::vector<fs::path>> find_large_files(
    const fs::path& directory, const std::uintmax_t size_limit,
    ErrorLog& log) {
    std::vector<fs::path> large_files;
    std::error_code ec;
    fs::recursive_directory_iterator it(
        directory, fs::directory_options::skip_permission_denied, ec);
    if (ec) {
        log.fatal("Cannot open directory " + directory.string() + ": " +
                  ec.message());
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
            log.error("Cannot read " + entry.path().string() + ": " +
                      ec.message());
            ec.clear();
        }

        // A failed increment leaves the iterator at end, so the scan stops
        // here but keeps the results found so far.
        const fs::path last = entry.path();
        it.increment(ec);
        if (ec) {
            log.error("Scan stopped after " + last.string() + ": " +
                      ec.message());
            break;
        }
    }
    return large_files;
}

void print_usage(std::ostream& out, const char* program) {
    out << "Usage: " << program
        << " [--min-size SIZE] [--log FILE] <directory>\n"
           "\n"
           "Lists regular files under <directory> larger than SIZE.\n"
           "\n"
           "  --min-size SIZE  size in bytes; K, M, G and T suffixes are\n"
           "                   powers of 1024 (default: 4G)\n"
           "  --log FILE       append errors to FILE instead of printing\n"
           "                   them to stderr\n"
           "  -h, --help       show this help\n";
}

int main(int argc, char* argv[]) {
    std::string min_size_text = "4G";
    std::optional<fs::path> log_path;
    std::optional<fs::path> directory;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(std::cout, argv[0]);
            return 0;
        } else if (arg == "--min-size" || arg == "--log") {
            if (i + 1 == argc) {
                std::cerr << "Error: " << arg << " needs a value\n";
                print_usage(std::cerr, argv[0]);
                return 1;
            }
            const std::string value = argv[++i];
            if (arg == "--min-size") {
                min_size_text = value;
            } else {
                log_path = value;
            }
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Error: unknown option " << arg << '\n';
            print_usage(std::cerr, argv[0]);
            return 1;
        } else if (!directory) {
            directory = arg;
        } else {
            std::cerr << "Error: unexpected argument " << arg << '\n';
            print_usage(std::cerr, argv[0]);
            return 1;
        }
    }

    if (!directory) {
        print_usage(std::cerr, argv[0]);
        return 1;
    }

    const std::optional<std::uintmax_t> size_limit = parse_size(min_size_text);
    if (!size_limit) {
        std::cerr << "Error: invalid size " << min_size_text
                  << " (expected a number with an optional K, M, G or T "
                     "suffix)\n";
        return 1;
    }

    ErrorLog log;
    if (log_path && !log.open(*log_path)) {
        std::cerr << "Error: cannot open log file " << log_path->string()
                  << '\n';
        return 1;
    }

    auto large_files_opt = find_large_files(*directory, *size_limit, log);
    if (!large_files_opt) {
        return 1;
    }

    const auto& large_files = *large_files_opt;
    std::cout << "Files larger than " << min_size_text << ":\n";
    for (const auto& file : large_files) {
        // Print the plain string; operator<< on a path adds quotes.
        std::cout << file.string() << '\n';
    }

    return 0;
}
