import platform
from setuptools import setup, find_packages, Extension
import io
import os
from setuptools.command.build_clib import build_clib
from setuptools.command.install_lib import install_lib
import subprocess
from distutils.util import get_platform
import sys

"""
Notes:
In order to upolad teh afdko wheel to testpypi to test the package, I had to
first update the Mac OSX system python ssl module with 'pip install pyOpenSSL
ndg-httpsclient pyasn1'. Otherwise, the command 'twine upload --repository-url
https://test.pypi.org/legacy/ dist/*' would fail with 'SSLError: [SSL: TLSV1_ALERT_PROTOCOL_VERSION]'
"""

"""
We need a customized version of the 'bdist_wheel' command, because otherwise the
wheel is identified as being non-platform specific. This is because the afdko
has no Python extensions and the command line tools are installed as data files.
"""
try:
    from wheel.bdist_wheel import bdist_wheel as _bdist_wheel
    class bdist_wheel(_bdist_wheel):
        def finalize_options(self):
            _bdist_wheel.finalize_options(self)
            self.root_is_pure = False
except ImportError:
    bdist_wheel = None
    print("setup requires that the Python package 'wheel' be installed. Try the command 'pip install wheel'.")
    sys.exit(1)

def getExecutableDir():
	"""
	Build source path on the FDK for the command-line tools.
	"""
	curSystem = platform.system()
	if curSystem == "Windows":
		binDir = "win"
	elif curSystem == "Linux":
		binDir = "linux"
	elif curSystem == "Darwin":
		binDir = "osx"
	else:
		raise KeyError("Do not recognize target OS: %s" % (sys.platform))
	platform_name = get_platform()
	return binDir, platform_name, curSystem
	
def compile(pgkDir):
	return
	binDir, platform_name, curSystem = getExecutableDir()
	programsDir = os.path.join(pgkDir, "Tools", "Programs")
	if binDir == 'osx':
		cmd = "sh BuildAll.sh"
	elif binDir == 'win':
		cmd = "BuildAll.cmd"
	elif binDir == 'linux':
		cmd = "sh BuildAllLinux.sh"
	curdir = os.getcwd()
	subprocess.check_call(cmd, cwd=programsDir, shell=True)
	os.chdir(curdir)

class CustomInstallLib(install_lib):
	"""Custom handler for the 'build_clib' command.
	Build the C programs before running the original bdist_egg.run()"""
	def run(self):
		pgkDir = self.distribution.package_dir['FDK']
		compile(pgkDir)
		install_lib.run(self)


binDir, platform_name, curSystem = getExecutableDir()
pkg_list =  find_packages()
python_bin = "%s/%s" % (sys.prefix, 'bin') # Build the isntall path for the tools.
classifiers=[
		'Development Status :: 5 - Production/Stable',
		'Intended Audience :: Developers',
		'Topic :: Software Development :: Build Tools',
		'License :: OSI Approved :: Apache Software License',
		'Programming Language :: Python :: 2.7',
	]
"""
Identify the dist build as being paltform specific.
"""
if curSystem == "Darwin":
	moreKeyWords = ['Operating System :: MacOS :: MacOS X',
					]
elif curSystem == "Windows":
	moreKeyWords = ['Operating System :: MacOS :: MacOS X',
					]
elif curSystem == "Linux":
	moreKeyWords = ['Operating System :: MacOS :: MacOS X',
					]
else:
	raise KeyError("Do not recognize target OS: %s" % (curSystem))
classifiers.extend(moreKeyWords)

# concatenate README.rst and NEWS.rest into long_description so they are
# displayed on the afdko project page on PyPI
# Copied fomr fonttools setup.py
with io.open("README.rst", "r", encoding="utf-8") as readme:
	long_description = readme.read()
long_description += "\nChangelog\n~~~~~~~~~\n\n"
with io.open("NEWS.rst", "r", encoding="utf-8") as changelog:
	long_description += changelog.read()

setup(name="afdko",
	  version="2.6.0.dev0",
	  description="Adobe Font Development Kit for OpenType",
	  long_description=long_description,
	  url='https://github.com/adobe-type-tools/afdko',
	  author='Read Roberts and many other font engineers',
	  author_email='rroberts@adobe.com',
	  license='Apache License, Version 2.0',
	  classifiers = classifiers,
	  keywords='font development tools',
	  platforms=[platform_name],
	  package_dir={'FDK': 'FDK'},
	  packages=pkg_list,
	  include_package_data = True,
	  package_data = {
		'SharedData': ['Adobe Cmaps/*'],
		'SharedData': ['CID charsets/*'],
		'SharedData': ['AGD.txt'],
	  },
	  python_requires='>=2.7,',
	  setup_requires=['wheel'],
	  install_requires=[
		  'fonttools>=3.19.0', #3.19.1
		  'booleanOperations>=0.7.1',
		  'fontMath>=0.4.4', # 4.4,
		  'robofab>=1.2.1',
		  'defcon>=0.3.5',
		  'mutatorMath>=2.1.0', #2.1.0
		  'ufolib>=2.1.1',
		  'ufonormalizer>=0.3.2',
	  ],
	  scripts=[\
                  'FDK/Tools/osx/autohintexe',
                  'FDK/Tools/osx/makeotfexe',
                  'FDK/Tools/osx/mergeFonts',
                  'FDK/Tools/osx/rotateFont',
                  'FDK/Tools/osx/sfntdiff',
                  'FDK/Tools/osx/sfntedit',
                  'FDK/Tools/osx/spot',
                  'FDK/Tools/osx/tx',
                  'FDK/Tools/osx/type1',
                  ],
	  entry_points={
		  'console_scripts': [
			  "autohint = FDK.Tools.SharedData.FDKScripts.autohint:main",
			  "buildCFF2VF = FDK.Tools.SharedData.FDKScripts.buildCFF2VF:run",
			  "buildMasterOTFs = FDK.Tools.SharedData.FDKScripts.buildMasterOTFs:main",
			  "compareFamily = FDK.Tools.SharedData.FDKScripts.CompareFamily:main",
			  "checkOutlinesUFO = FDK.Tools.SharedData.FDKScripts.CheckOutlinesUFO:main",
			  "copyCFFCharstrings = FDK.Tools.SharedData.FDKScripts.copyCFFCharstrings:run",
			  "kernCheck = FDK.Tools.SharedData.FDKScripts.kernCheck:run",
			  "makeotf = FDK.Tools.SharedData.FDKScripts.MakeOTF:main",
			  "makeInstancesUFO = FDK.Tools.SharedData.FDKScripts.makeInstancesUFO:main",
			  "otc2otf = FDK.Tools.SharedData.FDKScripts.otc2otf:main",
			  "otf2otc = FDK.Tools.SharedData.FDKScripts.otf2otc:main",
			  "stemHist = FDK.Tools.SharedData.FDKScripts.StemHist:main",
			  "ttxn = FDK.Tools.SharedData.FDKScripts.ttxn:main",
			  "charplot = FDK.Tools.SharedData.FDKScripts.ProofPDF:charplot",
			  "digiplot = FDK.Tools.SharedData.FDKScripts.ProofPDF:digiplot",
			  "fontplot = FDK.Tools.SharedData.FDKScripts.ProofPDF:fontplot",
			  "fontplot2 = FDK.Tools.SharedData.FDKScripts.ProofPDF:fontplot2",
			  "fontsetplot = FDK.Tools.SharedData.FDKScripts.ProofPDF:fontsetplot",
			  "hintplot = FDK.Tools.SharedData.FDKScripts.ProofPDF:hintplot",
			  "waterfallplot = FDK.Tools.SharedData.FDKScripts.ProofPDF:waterfallplot",
			  "clean_afdko = FDK.Tools.SharedData.FDKScripts.FDKUtils:clean_afdko",
			  "check_afdko = FDK.Tools.SharedData.FDKScripts.FDKUtils:check_afdko",
		  ],
	  },
	  cmdclass={'install_lib': CustomInstallLib, 'bdist_wheel': bdist_wheel},
	)
