# ibex - 6502 and Apex simulator

## Introduction

ibex is a 6502 simulator specifically intended to run Apex executables.
ibex is written in C++23 (supported by GCC).

ibex development is hosted at the
[ibex Github repository](https://github.com/brouhaha/ibex/).

## License

ibex is Copyright 2025 Eric Smith.

ibex is Free Software, licensed under the terms of the
[GNU General Public License 3.0](https://www.gnu.org/licenses/gpl-3.0.en.html)
(GPL version 3.0 only, not later versions).

## Apex History

The 6502 Group was founded in 1975, originally as a special interest group
of the Denver Amateur Computer Society. Due to the shortage of available
6502 development software, members of the 6502 group created editors,
assemblers, language interpreters, and even compilers for the 6502,
initially for homebrew systems, or systems from The Digital Group, but
eventually for the Apple II as well.

The XPL0 programming language was created by Peter J.R. Boyle, as was the
Apex operating system. The command interpreter ("executive") of Apex, as
well as many of the provided utility programs, were written in XPL0.

## 6502 simulation code

The simulation code has been written from scratch in C++. Currently only
the documented opcodes of the NMOS 6502 are simulated. Attempt to execute
an undocumented opcode will throw an exception, exiting the program.

The simulation code has been tested against [Klaus Dorman's 6502 instruction
set test suite](https://github.com/Klaus2m5/6502_65C02_functional_tests).

## Building ibex

Ibex requires the
[Magic Enum](https://github.com/Neargye/magic_enum)
header-only library.

The build system uses
[SCons](https://scons.org),
which requires
[Python](https://www.python.org/)
3.6 or later.

To build ibex, make sure the Magic Enum headers are in
your system include path, per their documentation. From the top level
directory (above the "src" directory), type "scons". The resulting
executable will be build/ibex.

## Running ibex

ibex is executed from a command line. There is a single mandatory
positional argument to provides the name of the Apex executable
(.SAV file) to be loaded and executed.

Command line ptions may be used to specify the executable format,
files for disk input and output
(Apex character device 3), and a file for printer output (Apex
character device 2).

* `-b` specfies that the executable is in ".BIN" format, rather than ".SAV"

* `-i <filename>` specifies the disk input file

* `-o <filename>` specifies the disk input file

* `-l <filename>` specifies the printer output file


The Apex asm65 assembler might be invoked to assemble a program foo by

```
ibex asm65.sav -i foo.p65 -o foo.bin -l foo.lst
```

The resulting foo.bin could be executed by

```
ibex -b foo.bin
```

## Limitations

* The Apex simulation in ibex is quite limited. ibex can load an
  Apex .SAV file, and simulates the Apex REENTER and KHAND entry
  points.
