[![Travis](https://travis-ci.org/adobe-type-tools/afdko.svg)](https://travis-ci.org/adobe-type-tools/afdko)
[![Appveyor](https://ci.appveyor.com/api/projects/status/qurx2si4x54b97mt/branch/master?svg=true)](https://ci.appveyor.com/project/adobe-type-tools/afdko/branch/master)
[![Codacy](https://api.codacy.com/project/badge/Grade/08ceff914833445685924ebb1f040070)](https://www.codacy.com/app/adobe-type-tools/afdko?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=adobe-type-tools/afdko&amp;utm_campaign=Badge_Grade)
[![Coverage](https://codecov.io/gh/adobe-type-tools/afdko/branch/master/graph/badge.svg)](https://codecov.io/gh/adobe-type-tools/afdko)
[![PyPI](https://img.shields.io/pypi/v/afdko.svg)](https://pypi.org/project/afdko)

Adobe Font Development Kit for OpenType (AFDKO)
===============================================

The AFDKO is a set of tools for building OpenType font files from
PostScript and TrueType font data.

This repository contains the data files, Python scripts, and sources for
the command line programs that comprise the AFDKO. The project uses the
[Apache 2.0 OpenSource license](LICENSE.md).

Please refer to the file
[AFDKO-Overview.html](https://rawgit.com/adobe-type-tools/afdko/master/afdko/AFDKO-Overview.html)
for a more detailed description of what is included in the package.

Major Changes
-------------

The AFDKO has been restructured so that it can be installed as a Python
package. It now depends on the user\'s Python interpreter, and no longer
contains its own Python interpreter. In order to do this, two
Adobe-owned, non-open source programs were dropped: **IS** and
**checkOutlines**. If these turn out to be sorely missed, an installer
for them will be added to the old Adobe AFDKO website. The current
intent is to migrate the many tests in checkOutlines to the newer
**checkOutlinesUFO** (which does work with OpenType and Type 1 fonts,
but currently does only overlap detection and removal, and a few basic
path checks).

Installation
------------

The AFDKO requires [Python](http://www.python.org/download) 2.7.x. It
does not yet support Python 3.x.

Releases are available on the [Python Package
Index](https://pypi.python.org/pypi/afdko) (PyPI) and can be installed
with [pip](https://pip.pypa.io).

### Installing

**Option 1 (Recommended)**

- Install [virtualenv](https://virtualenv.pypa.io):

        pip install --user virtualenv

- Create a virtual environment:

        python -m virtualenv afdko_env

- Activate the virtual environment:

    - macOS & Linux

            source afdko_env/bin/activate

    - Windows

            afdko_env\Scripts\activate.bat

- Install [afdko](https://pypi.python.org/pypi/afdko):

        pip install afdko

Installing the afdko inside a virtual environment prevents conflicts
between its dependencies and other modules installed globally.

**Option 2**

Install [afdko](https://pypi.python.org/pypi/afdko) globally:

    pip install --user afdko

### Updating

Use the `-U` (or `--upgrade`) option to update the afdko (and its
dependencies) to the newest stable release:

    pip install -U afdko

To get pre-release and in-development versions, use the `--pre` flag:

    pip install -U afdko --pre

### Uninstalling

To remove the afdko package use the command:

    pip uninstall afdko

### Comments

If you have both the FDK from the Adobe AFDKO web page installed, and
the new afdko package installed, the commands in the new afdko will take
precedence over commands in the older Adobe FDK, as the Python package
directory is added at the beginning of the PATH directory list, and the
old installer added the Adobe FDK directory to the end of the list.

Note that the PyPI installer will add the new adko package paths to the
start of your system PATH environment variable, and this is not undone
by the uninstaller. If you want to completely clean up, you will need to
change the PATH environment variable to remove the new afdko executable
directories. On the Mac, this means editing the line in your login file
that sets the PATH variable. On Windows, this means editing the PATH
environment variable in the System control panel.

You can download older versions of the tools from the [Adobe AFDKO
homepage](http://www.adobe.com/devnet/opentype/afdko.html). The tools IS
and checkOutlines are included in these downloads.

Build from Source
-----------------

In order to build afdko from source get the files from the [afdko github
repository](https://github.com/adobe-type-tools/afdko), cd to the
top-level directory of the afdko, and use the `setup.py` script:

    python setup.py install

And to be able to run this install command, you must first have
installed the development tools for your platform.

On the Mac, install these with:

    xcode-select --install

On Linux, install these with:

    apt-get -y install python2.7
    apt-get -y install python-pip
    apt-get -y install python-dev

On Windows, you need to download and install Visual C++ 6, and add all
the service packs.
