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
	Build source path on the afdko for the command-line tools.
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
		pgkDir = 'afdko'
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
scripts=[\
	  'afdko/Tools/osx/autohintexe',
	  'afdko/Tools/osx/makeotfexe',
	  'afdko/Tools/osx/mergeFonts',
	  'afdko/Tools/osx/rotateFont',
	  'afdko/Tools/osx/sfntdiff',
	  'afdko/Tools/osx/sfntedit',
	  'afdko/Tools/osx/spot',
	  'afdko/Tools/osx/tx',
	  'afdko/Tools/osx/type1',
		  ]
if curSystem == "Darwin":
	moreKeyWords = ['Operating System :: MacOS :: MacOS X',
					]
elif curSystem == "Windows":
	moreKeyWords = ['Operating System :: MacOS :: MacOS X',
					]
	scripts=[\
		  'afdko/Tools/osx/autohintexe.exe',
		  'afdko/Tools/osx/makeotfexe.exe',
		  'afdko/Tools/osx/mergeFonts.exe',
		  'afdko/Tools/osx/rotateFont.exe',
		  'afdko/Tools/osx/sfntdiff.exe',
		  'afdko/Tools/osx/sfntedit.exe',
		  'afdko/Tools/osx/spot.exe',
		  'afdko/Tools/osx/tx.exe',
		  'afdko/Tools/osx/type1.exe',
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
	  version="2.6.3",
	  description="Adobe Font Development Kit for OpenType",
	  long_description=long_description,
	  url='https://github.com/adobe-type-tools/afdko',
	  author='Read Roberts and many other font engineers',
	  author_email='rroberts@adobe.com',
	  license='Apache License, Version 2.0',
	  classifiers = classifiers,
	  keywords='font development tools',
	  platforms=[platform_name],
	  package_dir={'afdko': 'afdko'},
	  packages=pkg_list,
	  include_package_data = True,
	  zip_safe=False,
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
	  scripts=scripts,
	  entry_points={
		  'console_scripts': [
			  "autohint = afdko.Tools.SharedData.FDKScripts.autohint:main",
			  "buildCFF2VF = afdko.Tools.SharedData.FDKScripts.buildCFF2VF:run",
			  "buildMasterOTFs = afdko.Tools.SharedData.FDKScripts.buildMasterOTFs:main",
			  "compareFamily = afdko.Tools.SharedData.FDKScripts.CompareFamily:main",
			  "checkOutlinesUFO = afdko.Tools.SharedData.FDKScripts.CheckOutlinesUFO:main",
			  "copyCFFCharstrings = afdko.Tools.SharedData.FDKScripts.copyCFFCharstrings:run",
			  "kernCheck = afdko.Tools.SharedData.FDKScripts.kernCheck:run",
			  "makeotf = afdko.Tools.SharedData.FDKScripts.MakeOTF:main",
			  "makeInstancesUFO = afdko.Tools.SharedData.FDKScripts.makeInstancesUFO:main",
			  "otc2otf = afdko.Tools.SharedData.FDKScripts.otc2otf:main",
			  "otf2otc = afdko.Tools.SharedData.FDKScripts.otf2otc:main",
			  "stemHist = afdko.Tools.SharedData.FDKScripts.StemHist:main",
			  "ttxn = afdko.Tools.SharedData.FDKScripts.ttxn:main",
			  "charplot = afdko.Tools.SharedData.FDKScripts.ProofPDF:charplot",
			  "digiplot = afdko.Tools.SharedData.FDKScripts.ProofPDF:digiplot",
			  "fontplot = afdko.Tools.SharedData.FDKScripts.ProofPDF:fontplot",
			  "fontplot2 = afdko.Tools.SharedData.FDKScripts.ProofPDF:fontplot2",
			  "fontsetplot = afdko.Tools.SharedData.FDKScripts.ProofPDF:fontsetplot",
			  "hintplot = afdko.Tools.SharedData.FDKScripts.ProofPDF:hintplot",
			  "waterfallplot = afdko.Tools.SharedData.FDKScripts.ProofPDF:waterfallplot",
			  "clean_afdko = afdko.Tools.SharedData.FDKScripts.FDKUtils:clean_afdko",
			  "check_afdko = afdko.Tools.SharedData.FDKScripts.FDKUtils:check_afdko",
		  ],
	  },
	  cmdclass={'install_lib': CustomInstallLib, 'bdist_wheel': bdist_wheel},
	)
