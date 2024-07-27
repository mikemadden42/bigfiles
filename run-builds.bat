@echo off
setlocal enabledelayedexpansion

:: List of build types
set BUILD_TYPES=Debug Release ASAN UBSAN TSAN MSAN COVERAGE RelWithDebInfo

:: Directory for the build
set BUILD_DIR=build

:: Initialize counters and lists for build results
set /a success_count=0
set /a failure_count=0
set "successful_builds="
set "failed_builds="

:: Loop through each build type
for %%B in (%BUILD_TYPES%) do (
    echo Building for type: %%B

    :: Remove any existing build artifacts
    if exist %BUILD_DIR% rd /s /q %BUILD_DIR%
    mkdir %BUILD_DIR%
    cd %BUILD_DIR%

    :: Run CMake with the current build type
    cmake -DCMAKE_BUILD_TYPE=%%B ..

    :: Build the project
    if %errorlevel% equ 0 (
        msbuild dirsize.sln /p:Configuration=%%B
        if %errorlevel% equ 0 (
            echo %%B build succeeded.
            set "successful_builds=!successful_builds! %%B"
            set /a success_count+=1
        ) else (
            echo %%B build failed.
            set "failed_builds=!failed_builds! %%B"
            set /a failure_count+=1
        )
    ) else (
        echo CMake configuration for %%B failed.
        set "failed_builds=!failed_builds! %%B"
        set /a failure_count+=1
    )

    :: Go back to the root directory
    cd ..
)

:: Report the summary of builds
echo Build Summary:
echo Successful Builds: %success_count%
echo Failed Builds: %failure_count%

:: List successful build types
if not "!successful_builds!"=="" (
    echo Successful Build Types:
    for %%S in (%successful_builds%) do echo   - %%S
)

:: List failed build types if there were any failures
if not "!failed_builds!"=="" (
    echo Failed Build Types:
    for %%F in (%failed_builds%) do echo   - %%F
)

:: Exit with appropriate status code
if %failure_count% gtr 0 (
    exit /b 1
) else (
    exit /b 0
)
