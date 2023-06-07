# Chipdu++
Chipdu++ is a basic CHIP-8 Emulator written in C++.

## Overview
Chipdu++ is a foray into emulation and the concept of systems as a whole. Many CHIP-8 emulators have come before it, and I'm sure many are still to follow. I learn best by doing, thus, we are here.
The code is not very organized and likely not the most efficient in some places, but it has laid some very valuable groundwork for my knowledge.

In order to make up for lack of uniqueness, I've kept a lot of my comments in, explaining basic concepts of things like the opcodes, variables, purposes of certain functions, and otherwise verbose teachings and grievances.
I hope that they'll be useful in breaking down and explaining things to anyone who wishes to explore this kind of thing in the future.

### Downloads
Unlike (most) other CHIP-8 emulators, I plan to attempt to make builds available for all 3 big OSes, though sourcing a Mac may prove difficult. 
You'll be able to find them under the [Releases page](https://github.com/nn9dev/chipdu_plusplus/releases) when they're out.

## Usage
- Double click, run through your command line of choice, whatever you wanna do.  
- Press L to load a program. This should open a file picker. How many other CHIP-8 emulators do that, huh?  
- You can also pass a program via the CLI if you hate GUI's for some reason. Usage for that is as you'd expect, with obvious changes based on your OS:
```
chipdu_plusplus.exe D:\path\to\chip8\rom.c8
```
You can press ESC to exit, and `[` or `]` to Slow Down or Speed Up emulation respectively

### Keymap
I took this fun ASCII diagram from https://github.com/mir3z/chip8-emu. No other parts of their program were used in mine.
Original CHIP-8 keyboard layout is mapped to PC keyboard as follows:
```
|1|2|3|C|  =>  |1|2|3|4|
|4|5|6|D|  =>  |Q|W|E|R|
|7|8|9|E|  =>  |A|S|D|F|
|A|0|B|F|  =>  |Z|X|C|V|
```

## Building 
I'll be honest, I haven't tested this building thing too well, but it should work. The Makefiles were generated by [cbp2make](https://github.com/mirai-computing/cbp2make) and then slightly hand-edited.
The basic requirements are:
- a C++ compiler
- SDL2
- a noggin, as I probably messed something up
You can then compile by doing `make release -f Makefile.platform` where `platform` is one of `unix`, `mac`, or `windows`. I'm fairly certain the Mac and Unix files are the same but w/e.  

Depending on your build method you may need SDL2.dll or your system's equivalent in the same directory as the executable but I could be entirely wrong.
I've had one (1) report of loading a program via GUI not working on Linux, but I couldn't reproduce it myself. YMMV.

## Resources
A number of resources made this mini project possible. Other GitHub projects, google dot com, and Wikipedia included. These are the main ones:  
**portable-file-dialogs by samhocevar:** https://github.com/samhocevar/portable-file-dialogs  
**Lawrence Muller's How To Write An Emulator:** [https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/](https://web.archive.org/web/20230411125245/http://www.multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/)  
**The Wikipedia page for CHIP-8 opcodes:** https://en.wikipedia.org/wiki/CHIP-8#Opcode_table  
**IBM's C/C++ Standard Library documentation:** https://www.ibm.com/docs/en/i/7.1?topic=functions-cc-library  
<br>
<br>

---------------
Never stop learning, Never stop growing.  
Your world is what you make of it.  
With much love,  
9
