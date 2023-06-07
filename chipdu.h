#ifndef CHIPDU_H_INCLUDED
#define CHIPDU_H_INCLUDED

#pragma once    //i think i can do this...
// header files are hard man :(
#include <string>   //leave me alone this is necessary

class chipdu {
public:
    chipdu();   // empty contstuctor (init)

    void toggleDebugStuff();    //toggles "doDebugStuff" variable.
    void printEmulatorInformation();

    void doOneCycle();
    void debugRender();     ///the most helpful function around
    bool loadProgram(std::string filename);

    unsigned char screen[64 * 32];  // graphics are 1 bit (1 or 0) 64x32 b/w display
    unsigned char keypad[16];       // only 16 inputs, from 0x0 to 0xF
    bool drawFlag;      //so we know if we need to do any graphics operations

    void dumpMemory();
    void dumpRegisters();
    //spacing
private:
    unsigned short opcode;          // chip-8 has 35(?) opcodes which are all two bytes long.
                                    // To store the current opcode, we need a data type that allows us to store two bytes.
                                    // An unsigned short has the length of two bytes and therefore fits our needs.

    unsigned char memory[4096];     // 4kb of RAM in total. weird that we're interpreting as char
                                    // but i guess it makes sense

    unsigned char registers[16];    // 15 8-bit general purpose registers from 0-E, though you may notice it says 16.
                                    // The 16th register is used for the 'carry flag'
                                    // Eight bits is one byte so we can use an unsigned char for this purpose

    unsigned short indexRegister;   // 0x000-0xFFF, used as a free-floating variable
    unsigned short programCounter;  // 0x000-0xFFF (16 bits), keeps track of where we are in the program (will hold a hex value)

    /// chip-8 has no hardware registers or interrupts. wanna know what it does have though? timers.
    /// both count at 60hz, and when set above zero, they will count down TO zero
    /// The system's buzzer sounds whenever the sound timer reaches zero.
    unsigned char delay_timer;
    unsigned char sound_timer;

    unsigned short theStack[16];    // 16 levels of stack. used when we need to jump to keep our last position.
    unsigned short stackPointer;    // this keeps track of which stack level we're on.

    void init();    //teehee
};

/*
chip-8 memory map:

0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
0x200-0xFFF - Program ROM and work RAM
*/

#endif // CHIPDU_H_INCLUDED
