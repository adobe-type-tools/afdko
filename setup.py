import io
import os
import platform
import subprocess
import sys
import time
from distutils.util import get_platform

import setuptools.command.build_py
from setuptools import setup, find_packages

"""
We need a customized version of the 'bdist_wheel' command, because otherwise
the wheel is identified as being non-platform specific. This is because the
afdko has no Python extensions and the command line tools are installed as
data files.
"""
try:
    from wheel.bdist_wheel import bdist_wheel

    # noinspection PyClassicStyleClass,PyAttributeOutsideInit
    class CustomBDistWheel(bdist_wheel):
        def finalize_options(self):
            # noinspection PyArgumentList
            bdist_wheel.finalize_options(self)
            self.root_is_pure = False
except ImportError:
    print("setup requires that the Python package 'wheel' be installed. "
          "Try the command 'pip install wheel'.")
    sys.exit(1)


def get_executable_dir():
    """
    Build source path on the afdko for the command-line tools.
    """
    platform_system = platform.system()
    if platform_system == "Windows":
        bin_dir = "win"
    elif platform_system == "Linux":
        bin_dir = "linux"
    elif platform_system == "Darwin":
        bin_dir = "osx"
    else:
        raise KeyError(
            "Do not recognize target OS: {}".format(platform_system))
    return bin_dir


def compile_package(pkg_dir):
    bin_dir = get_executable_dir()
    programs_dir = os.path.join(pkg_dir, "Tools", "Programs")
    cmd = None
    if bin_dir == 'osx':
        cmd = "sh BuildAll.sh"
    elif bin_dir == 'win':
        cmd = "BuildAll.cmd"
    elif bin_dir == 'linux':
        cmd = "sh BuildAllLinux.sh"
    cur_dir = os.getcwd()
    assert cmd, 'Unable to form build command for this platform.'
    try:
        subprocess.check_call(cmd, cwd=programs_dir, shell=True)
    except subprocess.CalledProcessError:
        print('Error executing build command.')
        sys.exit(1)
    os.chdir(cur_dir)


# noinspection PyClassicStyleClass
class CustomBuild(setuptools.command.build_py.build_py):
    """Custom build command."""
    def run(self):
        pkg_dir = 'afdko'
        compile_package(pkg_dir)
        setuptools.command.build_py.build_py.run(self)


def _get_scripts():
    bin_dir = get_executable_dir()
    script_names = [
        'autohintexe', 'detype1', 'makeotfexe', 'mergeFonts', 'rotateFont',
        'sfntdiff', 'sfntedit', 'spot', 'tx', 'type1'
    ]
    if platform.system() == 'Windows':
        extension = '.exe'
    else:
        extension = ''

    scripts = ['afdko/Tools/{}/{}{}'.format(bin_dir, script_name, extension)
               for script_name in script_names]
    return scripts


def _get_console_scripts():
    script_entries = [
        ('autohint', 'autohint:main'),
        ('buildcff2vf', 'buildCFF2VF:run'),
        ('buildmasterotfs', 'buildMasterOTFs:main'),
        ('comparefamily', 'CompareFamily:main'),
        ('checkoutlinesufo', 'CheckOutlinesUFO:main'),
        ('copycffcharstrings', 'copyCFFCharstrings:run'),
        ('kerncheck', 'kernCheck:run'),
        ('makeotf', 'MakeOTF:main'),
        ('makeinstances', 'makeInstances:main'),
        ('makeinstancesufo', 'makeInstancesUFO:main'),
        ('otc2otf', 'otc2otf:main'),
        ('otf2otc', 'otf2otc:main'),
        ('stemhist', 'StemHist:main'),
        ('ttfcomponentizer', 'ttfcomponentizer:main'),
        ('ttxn', 'ttxn:main'),
        ('charplot', 'ProofPDF:charplot'),
        ('digiplot', 'ProofPDF:digiplot'),
        ('fontplot', 'ProofPDF:fontplot'),
        ('fontplot2', 'ProofPDF:fontplot2'),
        ('fontsetplot', 'ProofPDF:fontsetplot'),
        ('hintplot', 'ProofPDF:hintplot'),
        ('waterfallplot', 'ProofPDF:waterfallplot'),
        ('clean_afdko', 'FDKUtils:clean_afdko'),
        ('check_afdko', 'FDKUtils:check_afdko')
    ]
    scripts_path = 'afdko.Tools.SharedData.FDKScripts'
    scripts = ['{} = {}.{}'.format(name, scripts_path, entry)
               for name, entry in script_entries]
    return scripts


def _get_requirements():
    with io.open("requirements.txt", encoding="utf-8") as requirements:
        return requirements.read().splitlines()


def main():
    pkg_list = find_packages()
    classifiers = [
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Build Tools',
        'License :: OSI Approved :: Apache Software License',
        'Programming Language :: Python :: 2.7',
    ]

    platform_system = platform.system()
    if platform_system == "Darwin":
        more_keywords = ['Operating System :: MacOS :: MacOS X']
    elif platform_system == "Windows":
        more_keywords = ['Operating System :: Microsoft :: Windows']
    elif platform_system == "Linux":
        more_keywords = ['Operating System :: POSIX :: Linux']
    else:
        raise KeyError(
            "Do not recognize target OS: {}".format(platform_system))
    classifiers.extend(more_keywords)

    # concatenate README.rst and NEWS.rest into long_description so they are
    # displayed on the afdko project page on PyPI
    # Copied from fonttools setup.py
    with io.open("README.rst", encoding="utf-8") as readme:
        long_description = readme.read()
    with io.open("NEWS.rst", encoding="utf-8") as changelog:
        long_description += changelog.read()

    platform_name = get_platform()

    setup(name="afdko",
          version="2.6.26.dev" + time.strftime("%Y%m%d%H%M%S"),
          description="Adobe Font Development Kit for OpenType",
          long_description=long_description,
          url='https://github.com/adobe-type-tools/afdko',
          author='Adobe Type team & friends',
          author_email='afdko@adobe.com',
          license='Apache License, Version 2.0',
          classifiers=classifiers,
          keywords='font development tools',
          platforms=[platform_name],
          packages=pkg_list,
          include_package_data=True,
          zip_safe=False,
          python_requires='>=2.7',
          setup_requires=['wheel'],
          tests_require=[
              'pytest',
          ],
          install_requires=_get_requirements(),
          scripts=_get_scripts(),
          entry_points={
              'console_scripts': _get_console_scripts(),
          },
          cmdclass={
              'build_py': CustomBuild,
              'bdist_wheel': CustomBDistWheel},
          )


if __name__ == '__main__':
    main()

# PyPI deployment test 2018-04-09 15:15
