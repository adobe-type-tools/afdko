![AFDKO Test Suite](https://github.com/adobe-type-tools/afdko/workflows/AFDKO%20Test%20Suite/badge.svg)

[![Coverage](https://codecov.io/gh/adobe-type-tools/afdko/branch/develop/graph/badge.svg)](https://codecov.io/gh/adobe-type-tools/afdko/branch/develop)

[![PyPI](https://img.shields.io/pypi/v/afdko.svg)](https://pypi.org/project/afdko)

[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/adobe-type-tools/afdko.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/adobe-type-tools/afdko/context:cpp)
[![Language grade: Python](https://img.shields.io/lgtm/grade/python/g/adobe-type-tools/afdko.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/adobe-type-tools/afdko/context:python)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/adobe-type-tools/afdko.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/adobe-type-tools/afdko/alerts/) [![Join the chat at https://gitter.im/adobe-type-tools/afdko](https://badges.gitter.im/adobe-type-tools/afdko.svg)](https://gitter.im/adobe-type-tools/afdko?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Adobe Font Development Kit for OpenType (AFDKO)
===============================================

The AFDKO is a set of tools for building OpenType font files from
PostScript and TrueType font data.

This repository contains the data files, Python scripts, and sources for
the command line programs that comprise the AFDKO. The project uses the
[Apache 2.0 OpenSource license](LICENSE.md). Please note that the AFDKO
makes use of several dependencies, listed in the requirements.txt file,
which will automatically be installed if you install AFDKO with `pip`.
Most of these dependencies are BSD or MIT license, with the exception of
`tqdm`, which is [MPL 2.0](https://www.mozilla.org/en-US/MPL/2.0/).

Please refer to the
[AFDKO Overview](https://adobe-type-tools.github.io/afdko/AFDKO-Overview.html)
for a more detailed description of what is included in the package.

Please see the
[wiki](https://github.com/adobe-type-tools/afdko/wiki)
for additional information, such as links to reference materials and related
projects.

Installation
------------

The AFDKO requires [Python](http://www.python.org/download) 3.6
or later.

Releases are available on the [Python Package
Index](https://pypi.python.org/pypi/afdko) (PyPI) and can be installed
with [pip](https://pip.pypa.io).

Note for macOS users: we recommend that you do **not** use the system Python. Among other reasons, macOS ships with Python 2 and the latest version of the AFDKO is only available for Python 3. You can find instructions for using Brew to install Python 3 on macOS here: [Installing Python 3 on Mac OS X](https://docs.python-guide.org/starting/install3/osx/)

Note for all users: if you are in a Python 3 only environment, then the command `pip` is sufficient.  If you are in a mixed Python 2 and Python 3 environment, or you are unsure of your environment, then the command `pip3` ensures that you are using the Python 3 version of `pip`. It is for this reason that we have used `pip3` in the instructions below.

### Installing

**Option 1 (Recommended)**

- Create a virtual environment:

        python3 -m venv afdko_env

- Activate the virtual environment:

    - macOS & Linux

            source afdko_env/bin/activate

    - Windows

            afdko_env\Scripts\activate.bat

- Install [afdko](https://pypi.python.org/pypi/afdko):

        pip3 install afdko

Installing the **afdko** inside a virtual environment prevents conflicts
between its dependencies and other modules installed globally.

**Option 2**

Install [afdko](https://pypi.python.org/pypi/afdko) globally:

    pip3 install --user afdko

### Updating

Use the `-U` (or `--upgrade`) option to update the afdko (and its
dependencies) to the newest stable release:

    pip3 install -U afdko

To get pre-release and in-development versions, use the `--pre` flag:

    pip3 install -U afdko --pre

### Uninstalling

To remove the afdko package use the command:

    pip3 uninstall afdko

Build from source
-----------------

First you must have installed the development tools for your platform.

On the Mac, install these with:

    xcode-select --install

On Linux (Ubuntu 17.10 LTS or later), install these with:

    apt-get -y install python3.6
    apt-get -y install python-pip
    apt-get -y install python-dev

On Windows, you need Visual Studio 2017.

To build the **afdko** from source, clone the [afdko GitHub
repository](https://github.com/adobe-type-tools/afdko), ensure the `wheel`
module is installed (`pip3 install wheel`), then `cd` to the top-level
directory of the afdko, and run:

    pip3 install .

**Note**

It's not possible to install the afdko in editable/develop mode using
`pip3 install -e .` ; this is because the toolkit includes binary C executables
which setup.py tries to install in the bin/ (or Scripts/) folder, however
this process was only meant to be used with text-based scripts (either
written in Python or a shell scripting language). To work around this problem
(which really only impacts the few core afdko developers who need to get live
feedback as they modify the source files) you can use alternative methods like
exporting a PYTHONPATH, using a .pth file or similar hacks.
For further details read [this comment](https://github.com/adobe-type-tools/afdko/pull/677#issuecomment-436747212).

Major changes from version 2.5.x
--------------------------------

* The AFDKO has been restructured so that it can be installed as a Python
package. It now depends on the user's Python interpreter, and no longer
contains its own Python interpreter.

* Two programs, **IS** and **checkoutlines** were dropped because their source
code could not be open-sourced. These tools are available in [release version
2.5.65322 and older](https://github.com/adobe-type-tools/afdko/releases?after=2.6.22).

**Note**

If you install the old AFDKO as well as the new PyPI afdko package, the tools from
the newer version will take precedence over the older. This happens because pip
adds the afdko's package path at the beginning of the system's PATH environment
variable, whereas the old installer adds it at the end; this modification to PATH
is not undone by the uninstaller. If you want to completely remove the path to the
newer version, you will have to edit the PATH. On the Mac, this means editing the
line in your login file that sets the PATH variable. On Windows, this means editing
the PATH environment variable in the system's Control Panel.
