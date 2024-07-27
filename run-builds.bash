#!/bin/bash

# List of build types
BUILD_TYPES=("Debug" "Release" "ASAN" "UBSAN" "TSAN" "MSAN" "COVERAGE" "RelWithDebInfo")

# Directory for the build
BUILD_DIR="build"

# Create the build directory if it does not exist
mkdir -p $BUILD_DIR

# Initialize counters and arrays for build results
success_count=0
failure_count=0
successful_builds=()
failed_builds=()

# Loop through each build type
for build_type in "${BUILD_TYPES[@]}"; do
	echo "Building for type: $build_type"

	# Remove any existing build artifacts
	rm -rf $BUILD_DIR
	mkdir -p $BUILD_DIR
	cd $BUILD_DIR

	# Run CMake with the current build type
	cmake -DCMAKE_BUILD_TYPE=$build_type ..

	# Build the project
	if make; then
		echo "$build_type build succeeded."
		successful_builds+=($build_type) # Add the successful build type to the list
		((success_count++))
	else
		echo "$build_type build failed."
		failed_builds+=($build_type) # Add the failed build type to the list
		((failure_count++))
	fi

	# Go back to the root directory
	cd ..
done

# Report the summary of builds
echo "Build Summary:"
echo "Successful Builds: $success_count"
echo "Failed Builds: $failure_count"

# List successful build types
if [ ${#successful_builds[@]} -gt 0 ]; then
	echo "Successful Build Types:"
	for successful_build in "${successful_builds[@]}"; do
		echo "  - $successful_build"
	done
fi

# List failed build types if there were any failures
if [ ${#failed_builds[@]} -gt 0 ]; then
	echo "Failed Build Types:"
	for failed_build in "${failed_builds[@]}"; do
		echo "  - $failed_build"
	done
fi

# Exit with appropriate status code
if [ $failure_count -gt 0 ]; then
	exit 1
else
	exit 0
fi
