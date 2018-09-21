import distutils.command.build_scripts
import io
import os
import platform
import subprocess
import sys
from distutils import log
from distutils.dep_util import newer
from distutils.util import convert_path
from distutils.util import get_platform

import setuptools.command.build_py
import setuptools.command.install
from setuptools import setup

try:
    from wheel.bdist_wheel import bdist_wheel

    class CustomBDistWheel(bdist_wheel):
        """Mark the wheel as "universal" (both python 2 and 3), yet
        platform-specific, since it contains native C executables.
        """

        def finalize_options(self):
            bdist_wheel.finalize_options(self)
            self.root_is_pure = False

        def get_tag(self):
            return ('py2.py3', 'none',) + bdist_wheel.get_tag(self)[2:]

except ImportError:
    print("afdko: setup.py requires that the Python package 'wheel' be "
          "installed. Try the command 'pip install wheel'.")
    sys.exit(1)


class InstallPlatlib(setuptools.command.install.install):
    """This is to force installing all the modules to the non-pure, platform-
    specific lib directory, even though we haven't defined any 'ext_modules'.

    The distutils 'install' command, in 'finalize_options' method, picks
    either 'install_platlib' or 'install_purelib' based on whether the
    'self.distribution.ext_modules' list is not empty.

    Without this hack, auditwheel would flag the afdko wheel as invalid since
    it contains native executables inside the pure lib folder.

    TODO Remove this hack if/when in the future we install extension modules.
    """

    def finalize_options(self):
        setuptools.command.install.install.finalize_options(self)
        self.install_lib = self.install_platlib


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
            "afdko: Do not recognize target OS: {}".format(platform_system))
    return bin_dir


def compile_package(pkg_dir):
    bin_dir = get_executable_dir()
    programs_dir = 'c'
    cmd = None
    if bin_dir == 'osx':
        cmd = "sh buildall.sh"
    elif bin_dir == 'win':
        cmd = "buildall.cmd"
    elif bin_dir == 'linux':
        cmd = "sh buildalllinux.sh"
    cur_dir = os.getcwd()
    assert cmd, 'afdko: Unable to form build command for this platform.'
    try:
        subprocess.check_call(cmd, cwd=programs_dir, shell=True)
    except subprocess.CalledProcessError:
        print('afdko: Error executing build command.')
        sys.exit(1)
    os.chdir(cur_dir)


class CustomBuild(setuptools.command.build_py.build_py):
    """Custom build command."""
    def run(self):
        pkg_dir = 'afdko'
        compile_package(pkg_dir)
        setuptools.command.build_py.build_py.run(self)


class CustomBuildScripts(distutils.command.build_scripts.build_scripts):

    def copy_scripts(self):
        """Copy each script listed in 'self.scripts' *as is*, without
        attempting to adjust the !# shebang. The default build_scripts command
        in python3 calls tokenize to detect the text encoding, treating all
        scripts as python scripts. But all our 'scripts' are native C
        executables, thus the python3 tokenize module fails with SyntaxError
        on them. Here we just skip the if branch where distutils attempts
        to adjust the shebang.
        """
        self.mkpath(self.build_dir)
        outfiles = []
        updated_files = []
        for script in self.scripts:
            script = convert_path(script)
            outfile = os.path.join(self.build_dir, os.path.basename(script))
            outfiles.append(outfile)

            if not self.force and not newer(script, outfile):
                log.debug("afdko: not copying %s (up-to-date)", script)
                continue

            try:
                f = open(script, "rb")
            except OSError:
                if not self.dry_run:
                    raise
                f = None
            else:
                first_line = f.readline()
                if not first_line:
                    f.close()
                    self.warn("afdko: %s is an empty file (skipping)" % script)
                    continue

            if f:
                f.close()
            updated_files.append(outfile)
            self.copy_file(script, outfile)

        return outfiles, updated_files


def _get_scripts():
    script_names = [
        'detype1', 'makeotfexe', 'mergefonts', 'rotatefont',
        'sfntdiff', 'sfntedit', 'spot', 'tx', 'type1'
    ]
    if platform.system() == 'Windows':
        extension = '.exe'
    else:
        extension = ''

    scripts = ['c/build_all/{}{}'.format(script_name, extension)
               for script_name in script_names]
    return scripts


def _get_console_scripts():
    script_entries = [
        ('autohint', 'autohint:main'),
        ('buildcff2vf', 'buildcff2vf:run'),
        ('buildmasterotfs', 'buildmasterotfs:main'),
        ('comparefamily', 'comparefamily:main'),
        ('checkoutlinesufo', 'checkoutlinesufo:main'),
        ('makeotf', 'makeotf:main'),
        ('makeinstancesufo', 'makeinstancesufo:main'),
        ('otc2otf', 'otc2otf:main'),
        ('otf2otc', 'otf2otc:main'),
        ('otf2ttf', 'otf2ttf:main'),
        ('stemhist', 'stemhist:main'),
        ('ttfcomponentizer', 'ttfcomponentizer:main'),
        ('ttxn', 'ttxn:main'),
        ('charplot', 'proofpdf:charplot'),
        ('digiplot', 'proofpdf:digiplot'),
        ('fontplot', 'proofpdf:fontplot'),
        ('fontplot2', 'proofpdf:fontplot2'),
        ('fontsetplot', 'proofpdf:fontsetplot'),
        ('hintplot', 'proofpdf:hintplot'),
        ('waterfallplot', 'proofpdf:waterfallplot'),
    ]
    scripts_path = 'afdko'
    scripts = ['{} = {}.{}'.format(name, scripts_path, entry)
               for name, entry in script_entries]
    return scripts


def _get_requirements():
    with io.open("requirements.txt", encoding="utf-8") as requirements:
        return requirements.read().splitlines()


def main():
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
            "afdko: Do not recognize target OS: {}".format(platform_system))
    classifiers.extend(more_keywords)

    # concatenate README and NEWS into long_description so they are
    # displayed on the afdko project page on PyPI
    # Copied from fonttools setup.py
    with io.open("README.md", encoding="utf-8") as readme:
        long_description = readme.read()
    long_description += '\n'
    with io.open("NEWS.md", encoding="utf-8") as changelog:
        long_description += changelog.read()

    platform_name = get_platform()

    setup(name="afdko",
          use_scm_version=True,
          description="Adobe Font Development Kit for OpenType",
          long_description=long_description,
          long_description_content_type='text/markdown',
          url='https://github.com/adobe-type-tools/afdko',
          author='Adobe Type team & friends',
          author_email='afdko@adobe.com',
          license='Apache License, Version 2.0',
          classifiers=classifiers,
          keywords='font development tools',
          platforms=[platform_name],
          package_dir={'': 'python'},
          packages=['afdko'],
          include_package_data=True,
          package_data={
              'afdko': [
                  'resources/*.txt',
                  'resources/Adobe-CNS1/*',
                  'resources/Adobe-GB1/*',
                  'resources/Adobe-Japan1/*',
                  'resources/Adobe-Korea1/*'
              ],
          },
          zip_safe=False,
          python_requires='>=2.7',
          setup_requires=[
              'wheel',
              'setuptools_scm',
          ],
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
              'build_scripts': CustomBuildScripts,
              'bdist_wheel': CustomBDistWheel,
              'install': InstallPlatlib,
          },
          )


if __name__ == '__main__':
    main()
