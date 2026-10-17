# Install and set up Zucchini

[Overview](../README.md) · [Working with Zucchini](WorkingWithZucchini.md) · [Rules](Rules.md)

Zucchini is built from source and integrated into your CMake project. The repository
does not currently provide an installable CMake package; use a source checkout as
a subdirectory of your project.

## Requirements

- CMake 3.31 or newer. `zucchinify()` passes discovery arguments through `gtest_discover_tests(DISCOVERY_EXTRA_ARGS)`, which CMake added in 3.31.
- A C++17 compiler and a C compiler for dependencies.
- Git and network access for dependency downloads on the first configure.
- A build tool supported by CMake, such as Ninja, Make, or Visual Studio.

CMake fetches missing dependencies by default, including GoogleTest. Windows,
Linux, and macOS are covered by the project's CI configuration.

## Try the examples

```sh
git clone https://github.com/AnarchoSystems/Zucchini.git
cd Zucchini
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --parallel --output-on-failure
```

`--config Release` and `-C Release` select the configuration for generators such
as Visual Studio. `CMAKE_BUILD_TYPE` selects it for single-configuration generators
such as Make and Ninja.

The [examples](../Examples/README.md) include working scenarios and expected
failures checked by the test suite. To explore requirements and coverage, start
with the [business rules example](../Examples/Good/BusinessRules).

## Add Zucchini to your project

Place a checkout under `third_party/Zucchini` (for example, as a Git submodule).
In your project's `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.31)
project(MyProject LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

set(ZUCCHINI_BUILD_TESTS OFF CACHE BOOL "Build Zucchini's own tests")
add_subdirectory(third_party/Zucchini)

add_executable(CheckoutTest)
zucchinify(CheckoutTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE Checkout
)
zucchini_add_main(CheckoutTest Checkout)
```

The test target needs your `Checkout.h`, a YAML step manifest, and feature files.
The [workflow guide](WorkingWithZucchini.md) walks through creating all three.
`zucchinify()` generates the fixture interface and test source, links the runtime
and GoogleTest, and registers scenarios with CTest. `zucchini_add_main()` supplies
the entry point for that fixture.

Use the configure, build, and test commands above from your project directory.
During initial development, discovery can fail after adding missing step
definitions to the manifest. Review those definitions, implement the generated
methods, and rebuild as described in the workflow guide.

## Dependency options

CMake uses local source overrides first, then installed dependencies, then
fetches missing ones. For an offline setup, provide the dependencies and configure
with `-DZUCCHINI_FETCH_DEPENDENCIES=OFF`.

Local checkout overrides use `ZUCCHINI_<DEPENDENCY>_SOURCE_DIR`; see
[Dependencies.cmake](../cmake/Dependencies.cmake) for names and individual fetch
options. Set `-DZUCCHINI_VERBOSE_DEPENDENCIES=ON` to show dependency configuration
messages when troubleshooting.

Next: [write your first scenario](WorkingWithZucchini.md).
