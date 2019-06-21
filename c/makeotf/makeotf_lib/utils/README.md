# makeotf_lib utils

## generate_uniblock.py

This script generates the file `uniblock.h` from the Microsoft definition of the OS/2.ulUnicodeRange bits here:  
<https://docs.microsoft.com/en-us/typography/opentype/spec/os2#ur>  
(save this table as `os2_ur.txt` -- look at current file to be clear on which part to save)  
...and the Unicode consortium's `UnicodeData.txt` file available here:  
<ftp://ftp.unicode.org/Public/UNIDATA/UnicodeData.txt>  
(just save the file as `UnicodeData.txt`)  

To run the script:
```bash
$ python3 generate_uniblock.py > ../source/hotconv/uniblock.h
```
