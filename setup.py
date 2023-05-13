import io
import shutil
from skbuild import setup
# from skbuild.exceptions import SKBuildError
# from skbuild.cmaker import get_cmake_version


def main():
    classifiers = [
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Build Tools',
        'License :: OSI Approved :: Apache Software License',
        'Programming Language :: Python :: 3.9',
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX :: Linux',
    ]

    setup_requires = ['wheel', 'setuptools_scm', 'scikit-build', 'cython']

    # try:
    #     if LegacyVersion(get_cmake_version()) < LegacyVersion("3.16"):
    #         setup_requires.append('cmake')
    # except SKBuildError:
    #     setup_requires.append('cmake')

    setup_requires.append('cmake')

    try:
        if shutil.which('ninja') is None:
            setup_requires('ninja')
    except shutil.Error:
        setup_requires.append('ninja')

    with io.open("requirements.txt", encoding="utf-8") as requirements:
        install_requires = [rl.replace("==", ">=")
                            for rl in requirements.readlines()]

    # concatenate README and NEWS into long_description so they are
    # displayed on the afdko project page on PyPI
    # Copied from fonttools setup.py
    with io.open("README.md", encoding="utf-8") as readme:
        long_description = readme.read()
    long_description += '\n'
    with io.open("NEWS.md", encoding="utf-8") as changelog:
        long_description += changelog.read()

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
          package_dir={'': 'python'},
          packages=['afdko', 'afdko.pdflib', 'afdko.otfautohint'],
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
          python_requires='>=3.9',
          setup_requires=setup_requires,
          tests_require=[
              'pytest',
          ],
          install_requires=install_requires,
          entry_points={
              'console_scripts':
                  [   # "afdko=afdko.afdko:main",
                      "buildcff2vf=afdko.buildcff2vf:main",
                      "buildmasterotfs=afdko.buildmasterotfs:main",
                      "comparefamily=afdko.comparefamily:main",
                      "checkoutlinesufo=afdko.checkoutlinesufo:main",
                      "makeotf=afdko.makeotf:main",
                      "makeinstancesufo=afdko.makeinstancesufo:main",
                      "otc2otf=afdko.otc2otf:main",
                      "otf2otc=afdko.otf2otc:main",
                      "otf2ttf=afdko.otf2ttf:main",
                      "ttfcomponentizer=afdko.ttfcomponentizer:main",
                      "ttfdecomponentizer=afdko.ttfdecomponentizer:main",
                      "ttxn=afdko.ttxn:main",
                      "charplot=afdko.proofpdf:charplot",
                      "digiplot=afdko.proofpdf:digiplot",
                      "fontplot=afdko.proofpdf:fontplot",
                      "fontplot2=afdko.proofpdf:fontplot2",
                      "fontsetplot=afdko.proofpdf:fontsetplot",
                      "hintplot=afdko.proofpdf:hintplot",
                      "waterfallplot=afdko.proofpdf:waterfallplot",
                      "detype1=afdko._internal:detype1",
                      "addfeatures=afdko._internal:addfeatures",
                      "mergefonts=afdko._internal:mergefonts",
                      "rotatefont=afdko._internal:rotatefont",
                      "sfntdiff=afdko._internal:sfntdiff",
                      "sfntedit=afdko._internal:sfntedit",
                      "spot=afdko._internal:spot",
                      "tx=afdko._internal:tx",
                      "type1=afdko._internal:type1",
                      "otfautohint=afdko.otfautohint.__main__:main",
                      "otfstemhist=afdko.otfautohint.__main__:stemhist",
                  ],
          },)


if __name__ == '__main__':
    main()
