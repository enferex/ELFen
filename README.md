### ELFen: Extract and spell check read-only strings within ELF binaries.

#### What
ELFen is a strings-like utility for extracting printable strings from read-only
data sections of ELF files.  This utility also has the ability to spell check
these strings.

This tool intentionally does not process .debug symbols or relocation symbol
names.

#### Dependencies
If you want spell checking wizardry then you will need
GNU aspell <http://www.aspell.net> and any appropriate dictionaries.

#### Build
Simply run `make` to build without aspell support. Or, if you have aspell and
the proper dictionary package(s), then you can build-in the aspell support by
running `make aspell`.

#### Contact
mattdavis9@gmail.com (enferex)
