import io
import os
import platform
import subprocess
import sys
from distutils.util import get_platform

import setuptools.command.build_py
from setuptools import setup, find_packages

"""
Notes:
In order to upolad the afdko wheel to testpypi to test the package, I had to
first update the Mac OSX system python ssl module with 'pip install pyOpenSSL
ndg-httpsclient pyasn1'. Otherwise, the command 'twine upload --repository-url
https://test.pypi.org/legacy/ dist/*' would fail with
'SSLError: [SSL: TLSV1_ALERT_PROTOCOL_VERSION]'
"""

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
    cur_system = platform.system()
    if cur_system == "Windows":
        bin_dir = "win"
    elif cur_system == "Linux":
        bin_dir = "linux"
    elif cur_system == "Darwin":
        bin_dir = "osx"
    else:
        raise KeyError("Do not recognize target OS: %s" % sys.platform)
    platform_name = get_platform()
    return bin_dir, platform_name, cur_system


def compile_package(pkg_dir):
    if 0:
        print("Skipping compile - hard coded.")
        return
    bin_dir, platform_name, cur_system = get_executable_dir()
    programs_dir = os.path.join(pkg_dir, "Tools", "Programs")
    cmd = None
    if bin_dir == 'osx':
        cmd = "sh BuildAll.sh"
    elif bin_dir == 'win':
        cmd = "BuildAll.cmd"
    elif bin_dir == 'linux':
        cmd = "sh BuildAllLinux.sh"
    curdir = os.getcwd()
    assert cmd, 'Unable to form command for this platform.'
    subprocess.check_call(cmd, cwd=programs_dir, shell=True)
    os.chdir(curdir)


# noinspection PyClassicStyleClass
class CustomBuild(setuptools.command.build_py.build_py):
    """Custom build command."""
    def run(self):
        pkg_dir = 'afdko'
        compile_package(pkg_dir)
        setuptools.command.build_py.build_py.run(self)


def get_console_scripts():
    console_scripts = [
        "autohint = afdko.Tools.SharedData.FDKScripts.autohint:main",
        "buildcff2vf = afdko.Tools.SharedData.FDKScripts.buildCFF2VF:run",
        "buildmasterotfs = "
        "    afdko.Tools.SharedData.FDKScripts.buildMasterOTFs:main",
        "comparefamily = afdko.Tools.SharedData.FDKScripts.CompareFamily:main",
        "checkoutlinesufo = "
        "    afdko.Tools.SharedData.FDKScripts.CheckOutlinesUFO:main",
        "copycffcharstrings ="
        "    afdko.Tools.SharedData.FDKScripts.copyCFFCharstrings:run",
        "kerncheck = afdko.Tools.SharedData.FDKScripts.kernCheck:run",
        "makeotf = afdko.Tools.SharedData.FDKScripts.MakeOTF:main",
        "makeinstances = afdko.Tools.SharedData.FDKScripts.makeInstances:main",
        "makeinstancesufo ="
        "    afdko.Tools.SharedData.FDKScripts.makeInstancesUFO:main",
        "otc2otf = afdko.Tools.SharedData.FDKScripts.otc2otf:main",
        "otf2otc = afdko.Tools.SharedData.FDKScripts.otf2otc:main",
        "stemhist = afdko.Tools.SharedData.FDKScripts.StemHist:main",
        "ttxn = afdko.Tools.SharedData.FDKScripts.ttxn:main",
        "charplot = afdko.Tools.SharedData.FDKScripts.ProofPDF:charplot",
        "digiplot = afdko.Tools.SharedData.FDKScripts.ProofPDF:digiplot",
        "fontplot = afdko.Tools.SharedData.FDKScripts.ProofPDF:fontplot",
        "fontplot2 = afdko.Tools.SharedData.FDKScripts.ProofPDF:fontplot2",
        "fontsetplot = afdko.Tools.SharedData.FDKScripts.ProofPDF:fontsetplot",
        "hintplot = afdko.Tools.SharedData.FDKScripts.ProofPDF:hintplot",
        "waterfallplot ="
        "    afdko.Tools.SharedData.FDKScripts.ProofPDF:waterfallplot",
        "clean_afdko = afdko.Tools.SharedData.FDKScripts.FDKUtils:clean_afdko",
        "check_afdko = afdko.Tools.SharedData.FDKScripts.FDKUtils:check_afdko",
    ]
    return console_scripts


def main():
    bin_dir, platform_name, cur_system = get_executable_dir()
    pkg_list = find_packages()
    classifiers = [
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Build Tools',
        'License :: OSI Approved :: Apache Software License',
        'Programming Language :: Python :: 2.7',
    ]

    # Identify the dist build as being platform specific.
    scripts = [
        'afdko/Tools/%s/autohintexe' % bin_dir,
        'afdko/Tools/%s/detype1' % bin_dir,
        'afdko/Tools/%s/makeotfexe' % bin_dir,
        'afdko/Tools/%s/mergeFonts' % bin_dir,
        'afdko/Tools/%s/rotateFont' % bin_dir,
        'afdko/Tools/%s/sfntdiff' % bin_dir,
        'afdko/Tools/%s/sfntedit' % bin_dir,
        'afdko/Tools/%s/spot' % bin_dir,
        'afdko/Tools/%s/tx' % bin_dir,
        'afdko/Tools/%s/type1' % bin_dir,
    ]

    if cur_system == "Darwin":
        more_keywords = ['Operating System :: MacOS :: MacOS X']
    elif cur_system == "Windows":
        more_keywords = ['Operating System :: Microsoft :: Windows']
        scripts = [
            'afdko/Tools/win/autohintexe.exe',
            'afdko/Tools/win/detype1.exe',
            'afdko/Tools/win/makeotfexe.exe',
            'afdko/Tools/win/mergeFonts.exe',
            'afdko/Tools/win/rotateFont.exe',
            'afdko/Tools/win/sfntdiff.exe',
            'afdko/Tools/win/sfntedit.exe',
            'afdko/Tools/win/spot.exe',
            'afdko/Tools/win/tx.exe',
            'afdko/Tools/win/type1.exe',
        ]
    elif cur_system == "Linux":
        more_keywords = ['Operating System :: POSIX :: Linux']
    else:
        raise KeyError("Do not recognize target OS: %s" % cur_system)
    classifiers.extend(more_keywords)

    # concatenate README.rst and NEWS.rest into long_description so they are
    # displayed on the afdko project page on PyPI
    # Copied from fonttools setup.py
    with io.open("README.rst", encoding="utf-8") as readme:
        long_description = readme.read()
    with io.open("NEWS.rst", encoding="utf-8") as changelog:
        long_description += changelog.read()

    console_scripts = get_console_scripts()

    setup(name="afdko",
          version="2.6.25b2",
          description="Adobe Font Development Kit for OpenType",
          long_description=long_description,
          url='https://github.com/adobe-type-tools/afdko',
          author='Read Roberts and many other font engineers',
          author_email='rroberts@adobe.com',
          license='Apache License, Version 2.0',
          classifiers=classifiers,
          keywords='font development tools',
          platforms=[platform_name],
          packages=pkg_list,
          include_package_data=True,
          zip_safe=False,
          python_requires='>=2.7',
          setup_requires=['wheel'],
          install_requires=[
              'fonttools>=3.19.0',
              'booleanOperations>=0.7.1',
              'fontMath>=0.4.4',
              'robofab>=1.2.1',
              'defcon>=0.3.5',
              'mutatorMath>=2.1.0',
              'ufolib>=2.1.1',
              'ufonormalizer>=0.3.2',
          ],
          scripts=scripts,
          entry_points={
              'console_scripts': console_scripts,
          },
          cmdclass={'build_py': CustomBuild, 'bdist_wheel': CustomBDistWheel},
          )


if __name__ == '__main__':
    main()
