# FDK Build Notes

#### v1.3.0 May 2021

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

If the build is successful each program (e.g. makeotfexe) will be built as `build/bin/[program]`.  If you would like to install them in `/usr/local/bin` you can then run `cmake --build build -- install`.

### Noted CMake options

These options can be added to the end of the first CMake command above:

1. To change the "install prefix" (which is `/usr/local` by default) to `usr` add `-DCMAKE\_INSTALL\_PREFIX=/usr`
2. To build with debugging symbols and assertions add `-DCMAKE_BUILD_TYPE=Debug`
3. To build with `ninja` rather than `make` add `-GNinja`

#### Document Version History
Version 1.0.0  
Version 1.2.0 July 18 2019  
Version 1.2.1 - Convert to Markdown, update content - October 2019  
Version 1.3.0 - Document switch to CMake-driven builds
