

Adobe Font Development Kit for OpenType (AFDKO)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The AFDKO is a set of tools for building OpenType font files from PostScript and TrueType font data.

This directory tree contains the data files, Python scripts, Perl scripts, and
sources for the command line programs that comprise the AFDKO. The project uses the `Apache 2.0 OpenSource license <https://rawgit.com/adobe-type-tools/afdko/master/LICENSE.txt>`__.

Please refer to the file `AFDKO-Overview.html <https://rawgit.com/adobe-type-tools/afdko/master/FDK/AFDKO-Overview.html>`__ for a more detailed description of what is in the AFDKO.

Installation
~~~~~~~~~~~~

The AFDKO requires `Python <http://www.python.org/download/>`__ 2.7.x. It does not yet support Python 3.x.

The package is listed in the Python Package Index (PyPI), so you can
install it with `pip <https://pip.pypa.io>`__:

.. code:: sh

    pip install afdko

The available PyPi packages are for Mac OSX and Windows 64 bit Python.

You can also download and install older versions of the AFDKO from the `Adobe afdko home page <http://www.adobe.com/devnet/opentype/afdko.html>`__.


Build From Source
~~~~~~~~~~~~~~~~~~
In order to build and install the afdko from the `afdko github repository <https://github.com/adobe-type-tools/afdko>`__, get a copy from  the repository, cd to the top-level directory of the afdko, and use the setup.py script:

.. code:: sh

    python setup.py install

In to be able to run this install command, you must first have installed the development tools for your platform. On the Mac, install these with:

.. code:: sh

    xcode-select --install



On Linux, install these with:

.. code:: sh

    apt-get -y install python2.7
    apt-get -y install python-pip
    apt-get -y install python-dev


On Windows, you need to download and install Visual C++ 6, and add all the service packs.

