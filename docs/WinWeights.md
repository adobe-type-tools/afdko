# Practical Issues in Weight Setting and Style Linking (Windows):

#### Copyright 2016 Adobe Systems Incorporated. All rights reserved.

## Introduction
Although the Microsoft version of the TrueType specification explicitly allows for usWeightClass to be set to values from 100 to 900, TrueType fonts have historically always used values of 400 for a "regular" font in a Windows family with 700 for a "bold" style-linked font. Because of this history, in practice many or most Windows applications do not handle all possible values without problems. Using certain weightclasses, or style-lining certain weights, can result in fonts which are incorrectly "smeared bold," or cases where regardless of which font is selected (regular or bold) either only the regular or only the bold font appears.

In general, applications appear to divide into two groups: those which have problems with some weight settings and style-link combinations, and those which do not. The applications which have problems all appear to exhibit the same problems with the same fonts. Some examples of common problem applications include PageMaker 6.5.x, Word 97, and Word 2000.

To avoid such problems, you may wish to follow these guidelines. We have not done exhaustive testing with every possible value for weight or for style-linking, so it is conceivable that certain combinations which we believe are "functional" may have problems with some small number of applications.

## Usable values
For a font which is "regular" (not style-linked as the bold version of something else), any weightclass value from 250 to 1000 seems to work.
Having a weightclass of 100 or 200 can result in a "smear bold" or (unintentionally) returning the style-linked bold. Because of this, you may wish to manually override the weightclass setting for all "extra light," "ultra light" or "thin" fonts.

It is possible to use manual overrides to change values to any desired number. Be aware that some applications may begin to use weightclass in ways you have not anticipated. For example, newer Adobe applications (starting with Illustrator 9 and Photoshop 6) are using the weightclass where present to help determine sorting order of font submenus for all fonts sharing the same "preferred family." For this reason, we recommend that even if manual weightclass overrides are deemed necessary, font developers should preserve at least the relative weight relationships of all fonts within any given "preferred family."

Here are what ranges of weightclasses appear to work consistently as the "bold" style-links from any given base weightclass, along with what names are normally associated with the given weightclass values (association made by makeotf by default, based on the TrueType spec).

| REG  | BOLD RANGE (usable in increments of 50) |
|------|-----------------------------------------|
| 250: | 600-1000 Thin, extra light, ultra light at this value can bold to semibold or heavier |
| 300: | 600-950 Light can bold to medium, semi/demi, bold, extra-bold/ultra, or heavy |
| 350: | 600-900 (undefined) can bold to medium, semi/demi, bold, extra-bold/ultra, or heavy |
| 400: | 600-850 Regular can bold to semi/demi, bold, or extra-bold/ultra |
| 450: | 600-800 (undefined) can bold to semi/demi, bold, or extra-bold/ultra |
| 500: | 650-750 Medium can bold to bold |
| 550: | 600-800 (undefined) can bold to semi/demi, bold, or extra-bold/ultra |


---

#### Document Version History

Version 1.0 -  
Version 1.1 - Josh Hadley - Convert to Markdown, minor updates - October, 2019  
