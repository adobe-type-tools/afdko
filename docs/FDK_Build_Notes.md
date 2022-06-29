# FDK Build Notes

#### v1.3.1 April 2022

## Overview
These instructions are for users who wish to build the C programs in the AFDKO on their own. If you have obtained the AFDKO through [PyPI](https://pypi.org/project/afdko/) (`pip`), you can ignore this, as everything is pre-built for distribution on PyPI. These are really only for users who need to build on platforms that are not officially supported, or wish to make other custom changes.

## FDK directory tree

The FDK directory tree is pretty straightforward. The basic structure is:
```
afdko/
└── c/
    └── <component>/
        └── source/
```

When a tool uses a library there are additional directories for its include files and the library sources:
```
└── c/
    └── <component>/
        └── include/
        └── lib/<library>/source
        └── source/
```

## Special cases
`tx`, `mergefonts`, and `rotatefont` share a common set of libraries and resource files. These libraries are grouped under the shared directory in:
```
afdko/
└── c/
    └── shared/
        └── include/
        └── resource/
        └── source/<library>
```

## Build system

AFDKO now uses CMake as its build system. To build the programs with the default options run these commands in the top-level directory (above "c"):

```
cmake -S . -B build
cmake --build build
```

Note that unless you are using macOS or Windows you must have `libuuid` and its header files installed in order to build `makeotfexe`. These will typically be part of a package named `uuid-dev`, `libuuid-devel`, or `util-linux-libs`. (`libuuid` is a dependency of the Antlr 4 Cpp runtime, which is built by `cmake/ExternalAntlr4Cpp.cmake`. If you have build trouble after installing the library the [Antlr 4 Cpp runtime documentation](https://github.com/antlr/antlr4/tree/master/runtime/Cpp) may help.)

If the build is successful each program (e.g. makeotfexe) will be built as `build/bin/[program]`.  If you would like to install them in `/usr/local/bin` you can then run `cmake --build build -- install`.

AFDKO uses libxml2 for parsing in tx. If libxml2 is not found in the system, it will be installed through CMake externally in `ExternalLibXML2.cmake` and statically linked. This is usually the case for Windows. 
Currently, libxml2 will also be statically linked in Linux due to a [bug found in the linux python wheels when dynamically linked](https://github.com/adobe-type-tools/afdko/issues/1525).

### Noted CMake options

These options can be added to the end of the first CMake command above:

1. To change the "install prefix" (which is `/usr/local` by default) to `usr` add `-DCMAKE\_INSTALL\_PREFIX=/usr`
2. To build with debugging symbols and assertions add `-DCMAKE_BUILD_TYPE=Debug`
3. To build with `ninja` rather than `make` add `-GNinja`
4. To build XCode project files use `-GXcode`
5. More recent versions of Visual Studio (e.g. 2019) should recognize `CMakeLists.txt` files. Just open the main project folder.
6. When building directly on Windows you can specify the Visual Studio generator with `-G` and the platform with `-A`, as in `cmake -G "Visual Studio 16 2019" -A Win32`. See [Visual Studio Generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#visual-studio-generators) in the CMake documentation for more information.
7. To add a sanitizer add `-DADD_SANITIZER=[san]`, where `[san]` is `address`, `memory`, etc. (This currently only works with compilers that support GCC-style directives.) The top-level `CMakeLists.txt` will also honor an `ADD_SANITIZER` environment variable with the same values.

## Tests

Tests for both the C and Python programs are written in the Python `pytest` framework. If you have installed the `afdko` package using `pip` (or an equivalent) you can make `tests` your working directory and run `python -m pytest` to run the tests.

You can also run the tests before installation using CMake's `ctest` program or the `test` build target, assuming that you have both Python 3 and the `pytest` package installed. (For example, if you are building with ninja you can run `ninja test`.) Using `ctest` you can run the tests for one or more individual programs but not at a finer grain.

To run the tests manually/individually before installation you need to set a few environment variables. If the repository directory is `[AFDKO]` and the CMake build directory is `[AFDKO]/build` then the compiled programs will be in `[AFDKO]/build/bin`. Add this to the front of the `PATH` environment variable. Then add `[AFDKO]/python` to the front of the `PYTHONPATH` environment variable. Finally set the `AFDKO_TEST_SKIP_CONSOLE` variable to `True`. (The latter allows the tests to succeed without `console_script` wrappers for the Python EntryPoints.) The tests should now succeed or fail based on the current status of the build and python file changes.

## Troubleshooting

### `utfcpp` Timeout

The Antlr 4 build process downloads its `utfcpp` dependency using a `git` protocol URL. In some situations this action can time out. As a workaround you can substitute the more reliable `https` URL by running this command (which will modify your git configuration):

```
git config --global url.https://github.com/.insteadOf git://github.com/
```

### Antlr 4 Cpp runtime and Windows path lengths

You may have trouble building the Antlr 4 Cpp runtime on Windows due to git
being conservative about filename lengths. If you see an error like

```
  error: unable to create file googletest/xcode/Samples/FrameworkSample/WidgetFramework.xcodeproj/project.pbxproj: Filename too long
```

Try running:

```
git config --system core.longpaths true
```

#### Previously opened Windows Issues:
- Failed to build with Visual Studio 2019 [#1371](https://github.com/adobe-type-tools/afdko/issues/1371)
- Failed to install AFDKO in Windows 7 32bit [#1501](https://github.com/adobe-type-tools/afdko/issues/1502)
---
#### Document Version History

- Version 1.0.0
- Version 1.2.0 July 18 2019
- Version 1.2.1 - Convert to Markdown, update content - October 2019
- Version 1.3.0 - Document switch to CMake-driven builds - June 2021
- Version 1.3.1 - Add links to previously-opened Windows Issues - April 2022
