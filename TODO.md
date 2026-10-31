# TODO

Findings from a project review (2026-10-31). Suggested order: main.cc bugs first
(1, 2, 4, 3), then the CMake cleanup (9), then the `.bat` script (15).

## main.cc

### Bugs
- [ ] **1. Files reached through a symlink are listed twice.** `fs::is_regular_file(entry)`
      and `fs::file_size(entry)` follow symlinks, so a link to a big file and the file itself
      are both reported. Skip symlinks, or check with `entry.is_symlink()` /
      `entry.is_regular_file(ec)` without following links.
- [ ] **2. Errors don't show on the terminal.** Scanning a missing directory prints nothing
      and exits 1; the only explanation goes to the log file. Also write fatal errors to
      `std::cerr`.
- [ ] **3. Output paths are quoted.** `std::cout << file` uses `std::quoted`, so every line
      prints as `"..."`, which breaks piping into tools like `xargs`. Print `file.string()`.

### Robustness
- [ ] **4. One error mid-scan throws away every result.** An exception from the iterator's
      `operator++` (for example, a directory deleted during the scan) reaches the outer
      `catch`, which returns `nullopt` and discards everything found so far. Use the
      `error_code` overloads (`it.increment(ec)`, `entry.file_size(ec)`) and keep scanning.
- [ ] **5. Missing includes:** add `<cstdint>` (for `uintmax_t`) and `<string>` (for
      `log_error`).

### Usability
- [ ] **6.** Make the 4 GB limit configurable (e.g. `--min-size 4G`) instead of hard-coded.
- [ ] **7.** Make the log file argument optional, defaulting to stderr.
- [ ] **8.** Minor cleanup:
  - `log_error` is defined but `find_large_files` doesn't use it.
  - Log entries have no timestamps, even though the file is opened in append mode.
  - Use `'\n'` instead of `std::endl` to avoid flushing on every line.

## CMakeLists.txt

- [ ] **9. Sanitizer flags are added twice.** They are set in `CMAKE_CXX_FLAGS_<CONFIG>` and
      again by `add_custom_flags()`. That function checks `CMAKE_BUILD_TYPE`, which is empty
      for Visual Studio/Xcode generators, so it does nothing there. Delete the function and
      the empty `Release` branch; the `_<CONFIG>` variables already cover every generator.
- [ ] **10. Fix MSVC flags:**
  - `/DEBUG` is a linker flag; the compiler uses `/Zi`.
  - `/COVERAGE` doesn't exist.
  - `/fsanitize=undefined` isn't supported by MSVC.
  - `"/O2 /DEBUG"` is passed as one quoted argument.
- [ ] **11.** `set(CMAKE_CXX_FLAGS "-Wall ...")` overwrites user-supplied flags. Use
      `target_compile_options(bigfiles PRIVATE ...)` and add `/W4` for MSVC.
- [ ] **12.** The TSan/MSan "not supported" warnings print on every MSVC configure,
      whatever the build type.
- [ ] **13.** Raise `cmake_minimum_required` to at least 3.13 (needed for
      `target_link_options`).
- [ ] **14.** MSan only works with Clang on Linux, so the MSAN build always fails on macOS
      (AppleClang) and with GCC, and `run-builds.bash` always exits 1 there. Skip MSAN on
      unsupported compilers.

## Build scripts

- [ ] **15. `run-builds.bat` is broken.** It builds `dirsize.sln` (should be `bigfiles.sln`),
      and it reads `%errorlevel%` inside a `( ... )` block, where the value is fixed at parse
      time (needs `!errorlevel!`). `run-builds.ps1` already does the same job with
      `cmake --build`, so consider deleting the `.bat`.
- [ ] **16. `run-builds.bash`:** use `cmake --build .` instead of `make`, quote variables, and
      check that `cd` and the cmake configure step succeed.
- [ ] **17. `run-builds.ps1`:** remove the redundant build-directory creation before the
      loop, and stop capturing `cmake --build` output in `$buildResult` (it hides compiler
      errors).

## CI and docs

- [ ] **18.** Update `actions/checkout@v3` to v4+. Add a matrix for macOS/Windows and
      ASAN/UBSAN builds. Add at least one smoke test (`enable_testing()` + `add_test`)
      against a temp directory; right now `ctest` runs with no tests.
- [ ] **19.** Expand the README: build steps, usage, and the size threshold.
