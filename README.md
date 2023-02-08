![AFDKO Test Suite](https://github.com/adobe-type-tools/afdko/workflows/AFDKO%20Test%20Suite/badge.svg)

[![Coverage](https://codecov.io/gh/adobe-type-tools/afdko/branch/develop/graph/badge.svg)](https://codecov.io/gh/adobe-type-tools/afdko/branch/develop)

[![PyPI](https://img.shields.io/pypi/v/afdko.svg)](https://pypi.org/project/afdko)

[![Join the chat at https://gitter.im/adobe-type-tools/afdko](https://badges.gitter.im/adobe-type-tools/afdko.svg)](https://gitter.im/adobe-type-tools/afdko?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Adobe Font Development Kit for OpenType (AFDKO)
===============================================

The AFDKO is a set of tools for building OpenType font files from
PostScript and TrueType font data.

This repository contains the data files, Python scripts, and sources for
the command line programs that comprise the AFDKO. The project uses the
[Apache 2.0 Open Source license](https://opensource.org/licenses/Apache-2.0).
Please note that the AFDKO makes use of several dependencies, listed in the
requirements.txt file, which will automatically be installed if you install
AFDKO with `pip`. Most of these dependencies are BSD or MIT license, with
the exception of `tqdm`, which is [MPL 2.0](https://www.mozilla.org/en-US/MPL/2.0/).

Please refer to the
[AFDKO Overview](https://adobe-type-tools.github.io/afdko/AFDKO-Overview.html)
for a more detailed description of what is included in the package.

Please see the
[wiki](https://github.com/adobe-type-tools/afdko/wiki)
for additional information, such as links to reference materials and related
projects.

Installation
------------

The AFDKO requires [Python](http://www.python.org/download) 3.8
or later.
Regarding Python 3.11: while Python 3.11 itself is now relatively stable, we are waiting to let some known tool-chain problems resolve.

Releases are available on the [Python Package
Index](https://pypi.python.org/pypi/afdko) (PyPI) and can be installed
with [pip](https://pip.pypa.io).

Note for macOS users: we recommend that you do **not** use the system
Python. Among other reasons, some versions of macOS ship with Python 2
and the latest version of the AFDKO is only available for Python 3. You
can find instructions for using Brew to install Python 3 on macOS here:
[Installing Python 3 on Mac OS X](https://docs.python-guide.org/starting/install3/osx/).
Also: [pyenv](https://github.com/pyenv/pyenv) is a great tool for
installing and managing multiple Python versions on macOS.

Note for all users: we **STRONGLY** recommend the use of a Python virtual
environment ([`venv`](https://docs.python.org/3/library/venv.html))
and the use of `python -m pip install <package>` to install all packages
(not just AFDKO). Calling `pip install` directly can result in the
wrong `pip` being called, and the package landing in the wrong location.
The combination of using a `venv` + `python -m pip install` helps to ensure
that pip-managed packages land in the right place.

Note for Linux users (and users of other platforms that are not macOS or Windows): When there is not a pre-built "wheel" for your platform `pip` will attempt to build the C and C++ portions of the package from source. This process will only succeed if both the C and C++ development tools and libuuid are installed. See [build from source](#Build-from-source) below.

### Installing

**Option 1 (Recommended)**

- Create a virtual environment:

        python -m venv afdko_env

- Activate the virtual environment:

    - macOS & Linux

            source afdko_env/bin/activate

    - Windows

            afdko_env\Scripts\activate.bat

- Install [afdko](https://pypi.python.org/pypi/afdko):

        python -m pip install afdko

Installing the **afdko** inside a virtual environment prevents conflicts
between its dependencies and other modules installed globally.

**Option 2 (not recommended unless there is a global conflict)**

Local user installation [afdko](https://pypi.python.org/pypi/afdko) ([info](https://pip.pypa.io/en/stable/user_guide/?highlight=%E2%80%93%20user#user-installs)):

    python -m pip install --user afdko

### Updating

Use the `-U` (or `--upgrade`) option to update the afdko (and its
dependencies) to the newest stable release:

    python -m pip install -U afdko

To get pre-release and in-development versions, use the `--pre` flag:

    python -m pip install -U afdko --pre

### Uninstalling

To remove the afdko package use the command:

    python -m pip uninstall afdko

Build from source
-----------------

First you must have installed the development tools for your platform.

On macOS, install these with:

    xcode-select --install

On Linux (Ubuntu 17.10 LTS or later), install these with:

    apt-get -y install python3.8
    apt-get -y install python-pip
    apt-get -y install python-dev
    apt-get -y install uuid-dev

On other POSIX-like operating systems, `libuuid` and its header files
may be in a package named `libuuid-devel` or `util-linux-libs`. The
source code for `libuuid` is maintained in the
[util-linux repository](https://github.com/karelzak/util-linux).

On Windows, you need Visual Studio 2017 or later.


To build the **afdko** from source, clone the [afdko GitHub
repository](https://github.com/adobe-type-tools/afdko), ensure the `wheel`
module is installed (`python -m pip install wheel`), then `cd` to the top-level
directory of the afdko, and run:

    python -m pip install .

For further information on building from source see
[docs/FDK\_Build\_Notes.md](docs/FDK_Build_Notes.md).

**Note**

It's not possible to install the afdko in editable/develop mode using
`python -m pip install -e .` ; this is because the toolkit includes binary C executables
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
