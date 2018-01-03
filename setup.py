import platform
from setuptools import setup, find_packages, Extension
import io
import os
import setuptools.command.build_py
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
	print("Skipping compile - hard coded.")
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

class CustomBuild(setuptools.command.build_py.build_py):
	"""Custom build command."""

	def run(self):
		pgkDir = 'afdko'
		compile(pgkDir)
		setuptools.command.build_py.build_py.run(self)

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
Identify the dist build as being platform specific.
"""
scripts=[\
	  'afdko/Tools/%s/autohintexe' % (binDir),
	  'afdko/Tools/%s/detype1' % (binDir),
	  'afdko/Tools/%s/makeotfexe' % (binDir),
	  'afdko/Tools/%s/mergeFonts' % (binDir),
	  'afdko/Tools/%s/rotateFont' % (binDir),
	  'afdko/Tools/%s/sfntdiff' % (binDir),
	  'afdko/Tools/%s/sfntedit' % (binDir),
	  'afdko/Tools/%s/spot' % (binDir),
	  'afdko/Tools/%s/tx' % (binDir),
	  'afdko/Tools/%s/type1' % (binDir),
		  ]
if curSystem == "Darwin":
	moreKeyWords = ['Operating System :: MacOS :: MacOS X',
					]
elif curSystem == "Windows":
	moreKeyWords = ['Operating System :: MacOS :: MacOS X',
					]
	scripts=[\
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
	  version="2.6.17",
	  description="Adobe Font Development Kit for OpenType",
	  long_description=long_description,
	  url='https://github.com/adobe-type-tools/afdko',
	  author='Read Roberts and many other font engineers',
	  author_email='rroberts@adobe.com',
	  license='Apache License, Version 2.0',
	  classifiers = classifiers,
	  keywords='font development tools',
	  platforms=[platform_name],
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
			  "makeInstances = afdko.Tools.SharedData.FDKScripts.makeInstances:main",
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
	  cmdclass={'build_py': CustomBuild, 'bdist_wheel': bdist_wheel},
	)
