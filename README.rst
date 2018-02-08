|Travis Build Status| |Codacy| |Coverage Status| |PyPI|

Adobe Font Development Kit for OpenType (AFDKO)
===============================================

The AFDKO is a set of tools for building OpenType font files from PostScript and TrueType font data.

This repository contains the data files, Python scripts, and sources for the command line programs that comprise the AFDKO. The project uses the `Apache 2.0 OpenSource license`_.

Please refer to the file `AFDKO-Overview.html`_ for a more detailed description of what is included in the package.

.. _Apache 2.0 OpenSource license: https://rawgit.com/adobe-type-tools/afdko/master/LICENSE.md
.. _AFDKO-Overview.html: https://rawgit.com/adobe-type-tools/afdko/master/afdko/AFDKO-Overview.html


Major Changes
-------------

The AFDKO has been restructured so that it can be installed as a Python package. It now depends on the user's Python interpreter, and no longer contains its own Python interpreter.

In order to do this, two Adobe-owned, non-open source programs were dropped: **IS** and **checkOutlines**. If these turn out to be sorely missed, an installer for them will be added to the old Adobe AFDKO website.  The current intent is to migrate the many tests in checkOutlines to the newer **checkOutlinesUFO** (which does work with OpenType and Type 1 fonts, but currently does only overlap detection and removal, and a few basic path checks).


Installation
------------

The AFDKO requires Python_ 2.7.x. It does not yet support Python 3.x.

Releases are available on the `Python Package Index`_ (PyPI) and can be installed with pip_.
**The current PyPI package is a beta release for Mac OS X only.**

Installing
~~~~~~~~~~
**- Option 1 (Recommended)**

  Install `virtualenv`_:
   .. code:: sh

     pip install --user virtualenv


  Create a virtual environment:
   .. code:: sh

     python -m virtualenv afdko


  Activate the virtual environment:
  - macOS & Linux

     .. code:: sh

       source afdko/bin/activate

  - Windows

     .. code:: sh

       source afdko/bin/activate.bat


  Install `afdko`_:
   .. code:: sh

     pip install afdko


  Installing the afdko inside a virtual environment prevents conflicts between its dependencies and other modules installed globally.


**- Option 2**

  Install `afdko`_ globally:
   .. code:: sh

     pip install --user afdko


Updating
~~~~~~~~
Use the ``-U`` (or ``--upgrade``) option to update the afdko and its dependencies to the newest available release:

.. code:: sh

    pip install -U afdko


Uninstalling
~~~~~~~~~~~~
To remove the afdko package use the command:

.. code:: sh

    pip uninstall afdko


Comments
~~~~~~~~
If you have both the FDK from the Adobe AFDKO web page installed, and the new afdko package installed, the commands in the new afdko will take precedence over commands in the older Adobe FDK, as the Python package directory is added at the beginning of the PATH directory list, and the old installer added the Adobe FDK directory to the end of the list.

Note that the PyPI installer will add the new adko package paths to the start of your system PATH environment variable, and this is not undone by the uninstaller. If you want to completely clean up, you will need to change the PATH environment variable to remove the new afdko executable directories. On the Mac, this means editing the line in your login file that sets the PATH variable. On Windows, this means editing the PATH environment variable in the System control panel.

You can download older versions of the tools from the `Adobe AFDKO homepage`_.

.. _Python: http://www.python.org/download
.. _Python Package Index: https://pypi.python.org/pypi/afdko
.. _pip: https://pip.pypa.io
.. _virtualenv: https://virtualenv.pypa.io
.. _afdko: https://pypi.python.org/pypi/afdko
.. _Adobe AFDKO homepage: http://www.adobe.com/devnet/opentype/afdko.html


Build from Source
------------------
In order to build afdko from source get the files from the `afdko github repository`_, cd to the top-level directory of the afdko, and use the ``setup.py`` script:

.. code:: sh

    python setup.py install

And to be able to run this install command, you must first have installed the development tools for your platform.

On the Mac, install these with:

.. code:: sh

    xcode-select --install


On Linux, install these with:

.. code:: sh

    apt-get -y install python2.7
    apt-get -y install python-pip
    apt-get -y install python-dev


On Windows, you need to download and install Visual C++ 6, and add all the service packs.


.. _afdko github repository: https://github.com/adobe-type-tools/afdko

.. |Travis Build Status| image:: https://travis-ci.org/adobe-type-tools/afdko.svg
   :target: https://travis-ci.org/adobe-type-tools/afdko
.. |Codacy| image:: https://api.codacy.com/project/badge/Grade/673770d1687f4daca30c451bbb29c984
   :alt: Codacy Badge
   :target: https://www.codacy.com/app/cjchapman/afdko?utm_source=github.com&utm_medium=referral&utm_content=adobe-type-tools/afdko&utm_campaign=badger
.. |Coverage Status| image:: https://codecov.io/gh/adobe-type-tools/afdko/branch/master/graph/badge.svg
   :target: https://codecov.io/gh/adobe-type-tools/afdko
.. |PyPI| image:: https://img.shields.io/pypi/v/afdko.svg
   :target: https://pypi.python.org/pypi/afdko
