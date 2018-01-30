|Travis Build Status| |PyPI|

Adobe Font Development Kit for OpenType (AFDKO)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The AFDKO is a set of tools for building OpenType font files from PostScript and TrueType font data.

This directory tree contains the data files, Python scripts, Perl scripts, and
sources for the command line programs that comprise the AFDKO. The project uses the `Apache 2.0 OpenSource license <https://rawgit.com/adobe-type-tools/afdko/master/LICENSE.txt>`__.

Please refer to the file `AFDKO-Overview.html <https://rawgit.com/adobe-type-tools/afdko/master/afdko/AFDKO-Overview.html>`__ for a more detailed description of what is in the AFDKO.

Major Changes
~~~~~~~~~~~~~

The afdko has been restructured so that it can be installed as a Python package. It now depends on the user's Python interpreter, and no longer contains its own Python interpreter.

In order to do this, the two Adobe-owned, non-OpenSource programs were dropped: IS and checkOutlines. If these turn out to be sorely missed, an installer for them will be added to the old Adobe afdko web-site.  The current intent is to migrate the many tests in checkOutlines to the newer checkOutlinesUFO (which does work with OpenType and Type 1 fonts, but currently does only overlap detection and removal, and a few basic path checks).

Installation
~~~~~~~~~~~~

The AFDKO requires `Python <http://www.python.org/download/>`__ 2.7.x. It does not yet support Python 3.x.

The package is listed in the Python Package Index (PyPI), so you can
install it with `pip <https://pip.pypa.io>`__:

.. code:: sh

    pip install afdko

The current PyPi package is a beta release for Mac OS X.

You can remove the afdko package with the command:

.. code:: sh

    pip uninstall afdko

If you have both the FDK from the Adobe AFDKO web page installed, and and the new afdko package installed, the commands in the new afdko will take precedence over commands in the older Adobe FDK, as the Python package directory is added at the beginning of the PATH directory list, and the old installer added the Adobe FDK directory to the end of the list.

Note that the PyPi installer will add the new adko package paths to the start of your system PATH environment variable, and this is not undone by the uninstaller. If you want to completely clean up, you will need to change the PATH environment variable to remove the new afdko executable directories. On the Mac, this means editing the line in your login file that sets the PATH variable. On Windows, this means editing the PATH environment variable in the System control panel.

You can also download and install older versions of the AFDKO from the `Adobe afdko home page <http://www.adobe.com/devnet/opentype/afdko.html>`__.


Build From Source
~~~~~~~~~~~~~~~~~~
In order to build and install the afdko from the `afdko github repository <https://github.com/adobe-type-tools/afdko>`__, get a copy from  the repository, cd to the top-level directory of the afdko, and use the setup.py script:

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

.. |Travis Build Status| image:: https://travis-ci.org/adobe-type-tools/afdko.svg
   :target: https://travis-ci.org/adobe-type-tools/afdko
.. |PyPI| image:: https://img.shields.io/pypi/v/afdko.svg
   :target: https://pypi.org/project/afdko
