# FDK Build Notes

#### v1.2.1 October 2019

## Overview
These instructions are for users who wish to build the C programs in the AFDKO on their own. If you have obtained the AFDKO through [PyPI](https://pypi.org/project/afdko/) (`pip`), you can ignore this, as everything is pre-built for distribution on PyPI. These are really only for users who need to build on platforms that are not officially supported, or wish to make other custom changes.

## FDK build directory tree

The FDK build directory tree is pretty straightforward. The basic structure is:
```
afdko/
└── c/
    └── <component>/
        ├── build/
        │   └── <platform>/
        │       └── <compiler>/
        │           ├── Debug/
        │           └── Release/
        ├── exe/
        │   └── <platform>/
        │       ├── Debug/
        │       └── Release/
        └── source/
```

When a tool uses a library, then the project for the main tool contains the projects for all the libraries. The libraries are grouped under the main directory:
```
<component>/
└── <library group name>/
    ├── api/
    ├── build/
    ├── lib/
    └── source/
```
with sub-directories for `build/` and `lib/` being the same as for the program `build/` and `exe/` directories.

## Special cases
`tx`, `mergefonts`, and `rotatefont` share a common set of libraries and resource files. These libraries are grouped under the public directory in:
```
afdko/
└── c/
    └── public/
        └── lib/
            ├── api/
            ├── build/
            ├── lib/
            ├── resource/
            └── source/
```

## Build scripts

The collection of C programs in the FDK can be built by running one of the following scripts in `afdko/c/`:

  | Platform | script name        |
  |----------|--------------------|
  | Mac      | `buildall.sh`      |
  | Windows  | `buildall.cmd`     |
  | Linux    | `buildalllinux.sh` |


#### Document Version History
Version 1.0.0  
Version 1.2.0 July 18 2019  
Version 1.2.1 - Convert to Markdown, update content - October 2019  
