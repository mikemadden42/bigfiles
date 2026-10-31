# bigfiles

Find big files. `bigfiles` scans a directory tree and lists every regular file
larger than a given size (4 GiB by default).

## Usage

```
bigfiles [--min-size SIZE] [--log FILE] <directory>
```

| Option | Description |
|---|---|
| `--min-size SIZE` | List files larger than `SIZE` bytes. `K`, `M`, `G` and `T` suffixes are powers of 1024 and can be lowercase. Default: `4G`. |
| `--log FILE` | Append errors to `FILE`, with timestamps, instead of printing them to stderr. |
| `-h`, `--help` | Show help. |

Examples:

```sh
# Files over 4 GiB in your home directory
bigfiles ~

# Files over 500 MiB, logging unreadable files to scan.log
bigfiles --min-size 500M --log scan.log /data
```

Output is a header line followed by one path per line, unquoted:

```
Files larger than 500M:
/data/backups/disk.img
/data/video/raw footage.mov
```

To get just the paths, for example to pipe them into another command, skip the
header: `bigfiles ~ | tail -n +2`.

### What gets listed

- Only files **strictly larger** than the limit: with `--min-size 1K`, a file
  of exactly 1024 bytes isn't listed.
- Symlinks are skipped, so a file is never listed twice. Symlinked
  directories aren't followed.
- Directories you don't have permission to read are skipped.
- Files are listed in the order they're found, which isn't sorted.

### Errors and exit status

- If a file can't be read during the scan, the error is reported (to stderr,
  or to the `--log` file) and the scan carries on.
- If the directory can't be opened, or an option is invalid, `bigfiles` prints
  an error to stderr and exits with status 1. Errors that stop the program are
  printed to stderr even when `--log` is used.
- Otherwise it exits with status 0, even if some files couldn't be read.

### Upgrading from the old command line

Earlier versions took the log file as a required second argument:
`bigfiles <directory> <log_file>`. That form no longer works; use
`bigfiles --log <log_file> <directory>`.

## Building

You need CMake 3.13 or later and a C++17 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The program is built at `build/bigfiles`. With Visual Studio it is at
`build\Release\bigfiles.exe`. On macOS it's a universal binary for both Intel
and Apple silicon.

### Tests

```sh
cd build && ctest -C Release --output-on-failure
```

The smoke test needs bash, so it doesn't run on Windows.

### Build types

Besides the standard `Debug`, `Release` and `RelWithDebInfo`, there are
sanitizer and coverage build types. Each is only available with compilers that
support it; asking for an unsupported one stops CMake with an error.

| Build type | What it adds | GCC / Clang | MSVC |
|---|---|---|---|
| `ASAN` | AddressSanitizer | Yes | Yes |
| `UBSAN` | UndefinedBehaviorSanitizer | Yes | No |
| `TSAN` | ThreadSanitizer | Yes | No |
| `MSAN` | MemorySanitizer | Clang on Linux only | No |
| `COVERAGE` | `--coverage` instrumentation | Yes | No |

For example:

```sh
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=ASAN
cmake --build build-asan
```

To build every supported type one after another, run `./run-builds.bash`
(macOS and Linux) or `./run-builds.ps1` (Windows). Each one prints a summary
and exits with status 1 if any build fails. Both scripts delete and reuse the
`build` directory.

## License

MIT. See [LICENSE](LICENSE).
