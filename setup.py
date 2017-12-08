import platform
from setuptools import setup, find_packages, Extension
import os
from setuptools.command.build_clib import build_clib
from setuptools.command.install_lib import install_lib
import subprocess
from distutils.util import get_platform
import sys

def getExecutableDir():
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
	return binDir, platform_name
	
def compile(pgkDir):
	return
	binDir, platform_name = getExecutableDir()
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


binDir, platform_name = getExecutableDir()
pkg_list =  find_packages()
print 'pkg_list', pkg_list
python_bin = "%s/%s" % (sys.prefix, 'bin')

setup(name="afdko",
	  version="2.6.0.dev0",
	  description="Adobe Font Development Kit for OpenType",
	  url='https://github.com/adobe-type-tools/afdko',
	  author='Read Roberts and many other Adobe engineers',
	  author_email='readrob@pacbell.net',
	  license='Apache License, Version 2.0',
	  classifiers=[
		'Development Status :: 5 - Production/Stable',
		'Intended Audience :: Developers',
		'Topic :: Software Development :: Build Tools',
		'License :: OSI Approved ::Apache License, Version 2.0',
		'Programming Language :: Python :: 2.7',
	],
	  keywords='font development tools',
	  platforms=[platform_name],
	  package_dir={'FDK': 'FDK'},
	  packages=pkg_list,
	  include_package_data = True,
	  package_data = {
		# If any package contains *.txt files, include them:
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
	  data_files=[\
                  (python_bin, ['FDK/Tools/osx/autohintexe']),
                  (python_bin, ['FDK/Tools/osx/makeotfexe']),
                  (python_bin, ['FDK/Tools/osx/mergeFonts']),
                  (python_bin, ['FDK/Tools/osx/rotateFont']),
                  (python_bin, ['FDK/Tools/osx/sfntdiff']),
                  (python_bin, ['FDK/Tools/osx/sfntedit']),
                  (python_bin, ['FDK/Tools/osx/spot']),
                  (python_bin, ['FDK/Tools/osx/tx']),
                  (python_bin, ['FDK/Tools/osx/type1']),
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
		  ],
	  },
	  cmdclass={'install_lib': CustomInstallLib},
	)
