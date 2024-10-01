![AFDKO Test Suite](https://github.com/adobe-type-tools/afdko/workflows/AFDKO%20Test%20Suite/badge.svg)

[![Coverage](https://codecov.io/gh/adobe-type-tools/afdko/branch/develop/graph/badge.svg)](https://codecov.io/gh/adobe-type-tools/afdko/branch/develop)

[![PyPI](https://img.shields.io/pypi/v/afdko.svg)](https://pypi.org/project/afdko)

[![Join the chat at https://gitter.im/adobe-type-tools/afdko](https://badges.gitter.im/adobe-type-tools/afdko.svg)](https://gitter.im/adobe-type-tools/afdko?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Adobe Font Development Kit for OpenType (AFDKO)
===============================================

The AFDKO is a set of tools for building OpenType font files from
PostScript and TrueType font data.

Note: This version of the toolkit has been restructured and what was C code has been
partially ported to C++. For now these changes are considered experimental and
there may be significant bugs. However, the new code can do more or less what
the older code did and passes our test suite.

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

📣 Recent News
------------
**The Python port of psautohint was (re)integrated into the AFDKO repository as "otfautohint"**

More information can be found in [docs/otfautohint_Notes.md](docs/otfautohint_Notes.md)

Installation
------------

The AFDKO requires [Python](http://www.python.org/download) 3.9
or later. It should work with any Python > 3.9, but occasionally
tool-chain components and dependencies don't keep pace with major
Python releases, so there might be some lag time while they catch up.

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

    ```sh
    python -m venv afdko_env
    ```

- Activate the virtual environment:

    - macOS & Linux

        ```sh
        source afdko_env/bin/activate
        ```

    - Windows

        ```sh
        afdko_env\Scripts\activate.bat
        ```

- Install [afdko](https://pypi.python.org/pypi/afdko):

    ```sh
    python -m pip install afdko
    ```

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

    apt-get -y install python3.9
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

Developing
-----------------
If you'd like to develop & debug AFDKO using Xcode, run:

    CMake -G Xcode .

For further information on building from source see
[docs/FDK\_Build\_Notes.md](docs/FDK_Build_Notes.md).
