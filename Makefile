# This project uses CMake and Git sub-modules. This Makefile is just in place
# to make common tasks easier.

.PHONY: clean build

main: build
	./build/src/triangle_sandbox

test: build
	ctest --test-dir build

bench: build
	./build/src/triangle_sandbox_benchmarks

build: build/build.ninja
	cmake --build build

build/build.ninja:
	mkdir -p build
	cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug

clean:
	rm -rf build

sync:
	git submodule update --init --recursive -j 8
