#include <string>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include "chipdu.h"
#include "portable-file-dialogs.h"

bool doDebugStuff = false;      //this will ask to dump memory and other fun debug stuff

//unsigned int amountOfCycles = 0;
bool compatwith5565 = true;    //this is the boolean to enable the I = I + X + 1 change.

//Chip-8 fontset
unsigned char chip8_fontset[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

chipdu::chipdu()
{
    //do nothing, as everything is already properly initialized in the .h file
}

// this is REALLY messy but they're (mostly) in the order i wrote them in the header file so,,,
void chipdu::init()
{
    for(int i = 0; i<2048; i++)
        screen[i]=0;            // display array initialization (thanks 2D arrays...)

    for(int i = 0; i<16; i++)
        keypad[i]=0;            // keypad/input initialization

    opcode = 0;                 // opcode initialization (no opcode to start)

    for(int i = 0; i<4096; i++)
        memory[i]=0;            // memory initialization (funny, fills 4kb array w/ 0's)

    for(int i = 0; i<16; i++)
    {
        registers[i]=0;         // registers initialization
        theStack[i]=0;          // stack initialization
    }

    indexRegister   = 0;
    programCounter  = 0x200;    //system expects application to be at 0x200, so this is where we start
    stackPointer    = 0;

    // Load fontset
    for(int i = 0; i < 80; ++i)
        memory[i] = chip8_fontset[i];	// fontset intitialization

    delay_timer = 0;
    sound_timer = 0;

    drawFlag = true;
}

void chipdu::toggleDebugStuff()
{
    doDebugStuff = !doDebugStuff;
}

void chipdu::printEmulatorInformation()
{
    printf("Chipdu, a CHIP-8 Emulator by nn9.\n");
    printf("To exit the program, press ESCAPE.\n");
    printf("To load a new program, press the L key.\n");
    printf("To slow down or speed up emulation, use '[' and ']' respectively.\n");
    printf("Original CHIP-8 keyboard layout is mapped to PC keyboard as follows:\n");
    printf("|1|2|3|C|  =>  |1|2|3|4|\n");
    printf("|4|5|6|D|  =>  |Q|W|E|R|\n");
    printf("|7|8|9|E|  =>  |A|S|D|F|\n");
    printf("|A|0|B|F|  =>  |Z|X|C|V|\n");
}

bool chipdu::loadProgram(std::string filename)
{
    init();
    //printf("entered loadProgram!\n");
    //i'm doing filedialog idc about passing via command line
    if (filename.empty())
    {
        //invoke file dialog
        ///portable-file-dialogs.h throws an error: undefined reference to `CLSID_FileOpenDialog', must link com.dll

        auto selectedFile = pfd::open_file("Choose a game...",".",          // (string)title, (string)initial_path
        {
            "CHIP-8 Files", "*.c8 *.ch8",   // (vector)<std::string>filters
            "All Files",    "*"
        },
        pfd::opt::none);                // (pfd::opt)options
        //i swear this part is gonna give me a headache later...
        //update: it genuinely did. about an hour ago i took two ibuprofen for said headache.
        for (const auto &piece : selectedFile.result()) filename += piece;
        //printf("File name: %s\n", filename.c_str());  //moved to end of function because info gets cleared whoops
    }
    // prepare file
    FILE *fileWeWantToOpen = fopen(filename.c_str(), "rb");     //should work if passed by cli (not testing this teehee)
    if (fileWeWantToOpen == NULL)
    {
        printf("FILE ERROR!\n");
        return false;
    }

    /// LOAD FILE INTO MEMORY CODE BELOW
    // I/O in C/C++ is MENTAL
    // (actually i lied if i just used strict c++ or strict C it would be simpler)

    fseek(fileWeWantToOpen, 0, SEEK_END);   // file seek. look at file. "Change the current file position". jumps to end of file.
    long lSize = ftell(fileWeWantToOpen);   // find where we're looking at in the file. value returned is an offset relative to the beginning of the FILE.
    // since we've jumped to the end of the file, it'll report the "file size".
    rewind(fileWeWantToOpen);                          // repositions the file pointer associated with the parameter to the beginning of the file
    // A call to the rewind() function is the same as: `(void)fseek(stream, 0L, SEEK_SET);`
    printf("Filesize: %d\n", (int)lSize);   // print file size that we previously collected

    // Allocate memory to contain the whole file
    //printf("lSize: %ld\n", lSize);
    char *buffer = (char*)malloc(sizeof(char) *lSize);    // preparing a character buffer array for allocation...?

    //printf("buffer size: %i\n", (int)sizeof(buffer));       // this returns 8. this is because buffer is a pointer. please trust me.
    if (buffer == NULL)
    {
        printf("Unable to allocate memory!\n");
        return false;
    }

    // OKAY! SO! We have a FILE, we have a buffer, now we need to put the file. IN the buffer. which we can do by using fread();
    // size_t fread(void *buffer, size_t size, size_t count, FILE *stream);

    // The fread() function reads up to ^COUNT^ items of ^SIZE^ length from the input ^STREAM^ and stores them in the given ^BUFFER^.
    // The position in the file increases by the number of bytes read.
    size_t result = fread (buffer, 1, lSize, fileWeWantToOpen);
    //printf("fread result: %i\n", (int)sizeof(result));          // this ALSO returns 8.
    if ((long)result != lSize)    // if result is not the same size as what we expect (lSize, already checked), then...
    {
        printf("File read error!\n");
        return false;
    }
    // and FINALLY, before we copy over to our virtual memory, we need to see if the program can FIT in memory!
    if ((4096-512) > lSize)     //if (theoretical remaining memory) is greater than (theoretical file size), THEN...
    {
        for(int i = 0; i < lSize; i++)
            memory[i + 512] = buffer[i];    // 0x200 == 512 (we start the program at 0x200)
    }
    else    //copy to our memory buffer
    {
        printf("File is too big for 4kb memory! How did you do this!\n");
        return false;
    }

    //close file, free buffer
    //I feel like there should definitely be a way to keep track of our malloc()s...
    fclose(fileWeWantToOpen);
    free(buffer);

    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
    printEmulatorInformation();
    printf("File name: %s\n", filename.c_str());

    return true;
}

void chipdu::doOneCycle()
{
    ///Fetch, Decode, Execute, update timers.

    /** https://en.wikipedia.org/wiki/CHIP-8#Opcode_table
    NNN: address            (opcode & 0x0FFF)
    NN: 8-bit constant      (opcode & 0x00FF)
    N: 4-bit constant       (opcode & 0x0F00) or (opcode & 0x00F0) or (opcode & 0x000F). Shift as needed. (var >> 8) or (var >> 4).
    X and Y: 4-bit register identifier           (usually correspond to an "N" in the opcode)
    PC : Program Counter
    I : 16bit register (For memory address)
    VN: One of the 16 available "variables". N may be 0 to F in hexadecimal (16 values). THESE ARE OUR REGISTERS.
    **/

    // The system will fetch one opcode from the memory at the location specified by the program counter (pc).
    // In our Chip 8 emulator, data is stored in an array in which each address contains one byte.
    // As one opcode is 2 bytes long, we will need to fetch two successive bytes and merge them to get the actual opcode.
    // In order to merge both bytes and store them in an >>unsigned short<< (2 bytes datatype). We will use the bitwise OR operation.

    ///FETCH.
    opcode = memory[programCounter] << 8 | memory[programCounter + 1];
    // opcode (unsigned short, two bytes) equals memory[pc].
    // THEN, shift left 8 bits (move first opcode byte to first 8 bits of opcode variable)
    // NEXT, use the OR operator which will merge the second opcode byte to the second set of 8 bits of the opcode variable.
    // SO. we should now have a whole sixteen bits. two eight bit sets.
    // which is ONE opcode.

    ///DECODE
    switch (opcode & 0xF000)
    {
    case 0x0000:
        switch (opcode & 0x00FF) //leaves only the second byte
        {
        case 0x00E0: // 00E0: Clears the screen.
            //set screen[i] to all zeroes, set draw flag, increment pointer.
            for (int i = 0; i < 2048; ++i)
                screen[i] = 0x0;
            drawFlag = true;
            programCounter = programCounter + 2;

            break;
        case 0x00EE: // 00EE: Returns from a subroutine.
            //reduce Stack Pointer by one. zero out old SP. set Program Counter to reduced SP. increment PC to move to next instruction.
            --stackPointer;
            theStack[stackPointer + 1] = 0x0;
            programCounter = theStack[stackPointer];
            programCounter = programCounter + 2;

            break;
        }
        break; //END 0x0000

    case 0x1000: // 1NNN: Jumps to address NNN.
        //ignore stack, jump to address
        programCounter = (opcode & 0x0FFF);

        break; //END 0x1000

    case 0x2000: // 2NNN: Calls subroutine at NNN.
        //place our current address on the stack, increment the stack, jump to new address
        theStack[stackPointer] = programCounter;
        ++stackPointer;
        programCounter = (opcode & 0x0FFF);

        break; //END 0x2000

    case 0x3000: // 3XNN: Skips the next instruction if VX equals NN (usually the next instruction is a jump to skip a code block).
        if (registers[((opcode & 0x0F00) >> 8)] == (opcode & 0x00FF))
            programCounter = programCounter + 4;
        else
            programCounter = programCounter + 2;

        break; //END 0x3000

    case 0x4000: // 4XNN: Skips the next instruction if VX does not equal NN (usually the next instruction is a jump to skip a code block).
        if (registers[((opcode & 0x0F00) >> 8)] != (opcode & 0x00FF))
            programCounter = programCounter + 4;
        else
            programCounter = programCounter + 2;

        break; //END 0x4000

    case 0x5000: // 5XY0: Skips the next instruction if VX equals VY (usually the next instruction is a jump to skip a code block).
        if (registers[((opcode & 0x0F00) >> 8)] == registers[((opcode & 0x00F0) >> 4)])
            programCounter = programCounter + 4;
        else
            programCounter = programCounter + 2;

        break; //END 0x5000

    case 0x6000: // 6XNN: Sets VX to NN.
        registers[((opcode & 0x0F00) >> 8)] = (opcode & 0x00FF);
        programCounter = programCounter + 2;

        break; //END 0x6000

    case 0x7000: // 7XNN: Adds NN to VX (carry flag is not changed).
        registers[((opcode & 0x0F00) >> 8)] += (opcode & 0x00FF);
        programCounter = programCounter + 2;

        break; //END 0x7000

    /// I am FULLY aware that the below can be simplified with |=, &=, ^=, +=
    /// I am purposely choosing not to in hopes that this can be referenced by beginners
    case 0x8000: // 0x8XY#: bitwise/math operators
        switch (opcode & 0x000F)
        {
        case 0x0000: // 8XY0: Sets VX to the value of VY.
            registers[((opcode & 0x0F00) >> 8)] = registers[((opcode & 0x00F0) >> 4)];
            programCounter = programCounter + 2;

            break; //END 0x8XY0

        case 0x0001: // 8XY1: Sets VX to VX or VY. (bitwise OR operation)
            registers[((opcode & 0x0F00) >> 8)] = ((registers[((opcode & 0x0F00) >> 8)]) | (registers[((opcode & 0x00F0) >> 4)]));
            programCounter = programCounter + 2;

            break; //END 0x8XY1

        case 0x0002: // 8XY2: Sets VX to VX and VY. (bitwise AND operation)
            registers[((opcode & 0x0F00) >> 8)] = ((registers[((opcode & 0x0F00) >> 8)]) & (registers[((opcode & 0x00F0) >> 4)]));
            programCounter = programCounter + 2;

            break; //END 0x8XY2

        case 0x0003: // 8XY3: Sets VX to VX xor VY.
            registers[((opcode & 0x0F00) >> 8)] = ((registers[((opcode & 0x0F00) >> 8)]) ^ (registers[((opcode & 0x00F0) >> 4)]));
            programCounter = programCounter + 2;

            break; //END 0x8XY3

        case 0x0004: // 8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there is not.
            //subtracting V[x] from 255 (0xFF) and checking if that is less than V[y] guarantees we have not exceeded our addition limit
            registers[(opcode & 0x0F00) >> 8] = registers[(opcode & 0x0F00) >> 8] + registers[(opcode & 0x00F0) >> 4];

            if(registers[(opcode & 0x00F0) >> 4] > (0xFF - registers[(opcode & 0x0F00) >> 8]))
                registers[0xF] = 1; //carry
            else
                registers[0xF] = 0; //no carry

            programCounter = programCounter + 2;

            break; //END 0x8XY4

        case 0x0005: // 8XY5: VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't
            //like the above, but subtraction.
            if((registers[((opcode & 0x00F0) >> 4)]) > (registers[((opcode & 0x0F00) >> 8)]))
                registers[0xF] = 0; // there is a borrow (16th register)
            else
                registers[0xF] = 1;

            registers[((opcode & 0x0F00) >> 8)] -= registers[((opcode & 0x00F0) >> 4)];
            programCounter = programCounter + 2;

            break; //END 0x8XY5

        case 0x0006: // 8XY6: Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
            registers[0xF] = registers[(opcode & 0x0F00) >> 8] & 0x1;   // VF = V[X], BUT mask V[X] by '0x1' since it will
                                                                        // preserve LEAST significant >>bit<< (and that's what we want)
            registers[((opcode & 0x0F00) >> 8)] = (registers[((opcode & 0x0F00) >> 8)] >> 1);
            programCounter = programCounter + 2;

            break; //END 0x8XY6

        case 0x0007: // 8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there is not.
             //if VY is greater than VX, we will have borrowed (or have to borrow)
             if(registers[((opcode & 0x0F00) >> 8)] > registers[((opcode & 0x00F0) >> 4)])
                registers[0xF] = 0; //there is a borrow (16th register)
             else
                registers[0xF] = 1; //no borrow (1)

             registers[((opcode & 0x0F00) >> 8)] = (registers[((opcode & 0x00F0) >> 4)] - registers[((opcode & 0x0F00) >> 8)]);
             programCounter = programCounter+2;

            break; //END 0x8XY7

        case 0x000E: // 8XYE: Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
            registers[0xF] = (registers[((opcode & 0x0F00) >> 8)] >> 7);    //VF = V[X], BUT shift V[x] right by 7, which will
                                                                            //preserve MOST significant bit (and that's what we want)
            registers[((opcode & 0x0F00) >> 8)] = ((registers[((opcode & 0x0F00) >> 8)]) << 1);
            programCounter = programCounter + 2;

            break; //END 0x8XY6

        default:
            //if you see this line ever... uhh... i dunno man... bad rom?
            printf("Bad 0x8XY# opcode! Opcode is %X\n", opcode);
            break;
        }
        break; //END 0x8000

    case 0x9000: // 9XY0: Skips the next instruction if VX does not equal VY. (Usually the next instruction is a jump to skip a code block)
        if (registers[(opcode & 0x0F00) >> 8] != registers[(opcode & 0x00F0) >> 4])
            programCounter = programCounter + 4;
        else
            programCounter = programCounter + 2;

        break; //END 0x9000

    case 0xA000: // ANNN: Sets I to the address NNN.
        indexRegister = (opcode & 0x0FFF);
        programCounter = programCounter + 2;

        break; //END 0xA000

    case 0xB000: // BNNN: Jumps to the address NNN plus V0.
        programCounter = ((opcode & 0x0FFF) + registers[0]);

        break; //END 0xB000

    case 0xC000: // CXNN: Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
        //V[x] = rand() & NN
        registers[(opcode & 0x0F00) >> 8] = ((rand() % (0xFF + 1)) & (opcode & 0x00FF));
        programCounter = programCounter + 2;

        break; //END 0xC000

    ///I will be unapolagetically stating that this is (((NOT))) my block of code.
    ///credit goes to Lawrence Muller
    ///http://www.multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/
    case 0xD000: // DXYN: Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
    {
        unsigned short x = registers[(opcode & 0x0F00) >> 8];
        unsigned short y = registers[(opcode & 0x00F0) >> 4];
        unsigned short height = opcode & 0x000F;
        unsigned short pixel;

        registers[0xF] = 0;
        for (int yline = 0; yline < height; yline++)
        {
            pixel = memory[indexRegister + yline];
            for (int xline = 0; xline < 8; xline++)
            {
                if ((pixel & (0x80 >> xline)) != 0)
                {
                    if (screen[(x + xline + ((y + yline) * 64))] == 1)
                    {
                        registers[0xF] = 1;
                    }
                    screen[x + xline + ((y + yline) * 64)] ^= 1;
                }
            }
        }

        drawFlag = true;
        programCounter += 2;
    }
    break; //END 0xD000

    case 0xE000:
        switch (opcode & 0x00FF)
        {
        case 0x009E: // EX9E: Skips the next instruction if the key stored in VX is pressed (usually the next instruction is a jump to skip a code block).
            if (keypad[registers[((opcode & 0x0F00) >> 8)]] != 0)
                programCounter = programCounter + 4;
            else
                programCounter = programCounter + 2;

            break; //END EX9E

        case 0x00A1: // EXA1: Skips the next instruction if the key stored in VX is NOT pressed (usually the next instruction is a jump to skip a code block).
            if (keypad[registers[((opcode & 0x0F00) >> 8)]] == 0)
                programCounter = programCounter + 4;
            else
                programCounter = programCounter + 2;

            break; //END EXA1
        default:
            printf("Bad 0xE000 opcode! Instruction is: %X\n", opcode);
            break;
        }

        break; //END 0xE000

    case 0xF000:
        switch (opcode & 0x00FF)
        {
        case 0x0007: // FX07: Sets VX to the value of the delay timer.
            registers[(opcode & 0x0F00) >> 8] = delay_timer;
            programCounter = programCounter + 2;

            break; //END 0xFX07

        case 0x000A: // FX0A: A key press is awaited, and then stored in VX (blocking operation, all instruction halted until next key event).
        {
            bool keyPressed = false;
            for (int i = 0; i < 16; i++)
            {
                if (keypad[i] != 0)
                {
                    registers[(opcode & 0x0F00) >> 8] = i;
                    keyPressed = true;
                }
            }

            // if no key pressed, return. this will attempt the cycle again.
            if (!keyPressed)
                return; //might should be break?

            programCounter = programCounter + 2;
        }
        break; //END 0xFX0A

        case 0x0015: //FX15: Sets the delay timer to VX.
            delay_timer = registers[(opcode & 0x0F00) >> 8];
            programCounter = programCounter + 2;

            break; //END 0xFX15

        case 0x0018: //FX18: Sets the sound timer to VX.
            sound_timer = registers[(opcode & 0x0F00) >> 8];
            programCounter = programCounter + 2;

            break; //END 0xFX18

        case 0x001E: //FX1E: Adds VX to I. VF is not affected.
            // VF is set to 1 when range overflow (I+VX>0xFFF), and 0
            // when there isn't.
            /*
            NOTE:
            Most CHIP-8 interpreters' FX1E instructions do not affect VF, with one exception:
            the CHIP-8 interpreter for the Commodore Amiga sets VF to 1 when there is a range overflow (I+VX>0xFFF),
            and to 0 when there is not.[15]
            The only known game that depends on this behavior is Spacefight 2091!, while at least one game, Animal Race,
            depends on VF not being affected.
            */
            if (indexRegister + registers[(opcode & 0x0F00) >> 8] > 0xFFF)
                registers[0xF] = 1;
            else
                registers[0xF] = 0;
            indexRegister += registers[(opcode & 0x0F00) >> 8];
            programCounter += 2;

            break; //END 0xFX1E

        case 0x0029: //FX29: Sets I to the location of the sprite for the character in VX.
            indexRegister = registers[(opcode & 0x0F00) >> 8] * 0x5;
            programCounter = programCounter + 2;

            break; //END 0xFX29

        case 0x0033: //FX33: Stores the binary-coded decimal representation of VX, with the hundreds digit in memory at location in I.
            /**
            Binary Coded Decimal Representation (BCDR) is, put simply,
            a binary representation not of the whole NUMBER, but of each DIGIT.
            In standard binary, the number 1895 is represented as '0111 0110 0111'.
            The BCDR is '0001 1000 1001 0101', with each digit being represented by its own set of four bits.
            **/

            // Extract hundreds digit
            memory[indexRegister] = (registers[(opcode & 0x0F00) >> 8]) / 100;
            // Extract tens digit
            memory[indexRegister + 1] = ((registers[(opcode & 0x0F00) >> 8]) / 10) % 10;
            // Extract ones digit
            memory[indexRegister + 2] = (registers[(opcode & 0x0F00) >> 8]) % 10;
            programCounter = programCounter + 2;

            break; //END 0xFX33

        case 0x0055: //FX55: Stores from V0 to VX (including VX) in memory, starting at address I.
            //The offset from I is increased by 1 for each value written, but I itself is left unmodified.
            for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
            {
                memory[indexRegister + i] = registers[i];
            }
            ///some sources say the original interpreter did I = I + X + 1 after this opcode. I'll add this as a setting.
            if (compatwith5565)
                indexRegister += (((opcode & 0x0F00) >> 8) + 1);

            programCounter = programCounter + 2;

            break; //END 0xFX55

        case 0x0065: //FX65: Fills from V0 to VX (including VX) with values from memory, starting at address I.
            //The offset from I is increased by 1 for each value read, but I itself is left unmodified.
            for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++) // from V0 to VX
            {
                registers[i] = memory[indexRegister + i];
            }
            ///some sources say the original interpreter did I = I + X + 1 after this opcode. I'll add this as a setting.
            if (compatwith5565)
                indexRegister += (((opcode & 0x0F00) >> 8) + 1);

            programCounter = programCounter + 2;

            break; //END 0xFX65
        }

        break; //END 0xF000

    default:
        printf("Bad opcode! Instruction is: %X\n", opcode);
    }

    //update timers
    if (delay_timer != 0)
    {
        --delay_timer;
        //printf("delay_timer hit!\n");
    }
    //
    if (sound_timer != 0)
    {
        std::cout << "beep!" << '\7' << std::endl;
        //std::cout<<"beep!"<<'\a'<<std::endl;  //idk it's one of these
        //std::cout<<"beep!"<<7<<std::endl;
        --sound_timer;
    }

} //END chipdu::doCycle

//straight up stole this but y'know might be helpful. supposed to print a mock graphics array in the terminal.
void chipdu::debugRender()
{
    // Draw
    for(int y = 0; y < 32; ++y)
    {
        for(int x = 0; x < 64; ++x)
        {
            if(screen[(y*64) + x] == 0)
                printf("O");
            else
                printf(" ");
        }
        printf("\n");
    }
    printf("\n");
    std::cout<<"place in ROM is 0x"<<std::hex<<(programCounter - 0x200)<<std::endl;
    std::cout<<"opcode: "<<std::hex<<(int)opcode<<std::endl;
    //Sleep(3500);
    system("cls");
}

void chipdu::dumpMemory()
{
    std::cout<<"place in ROM is 0x"<<std::hex<<(programCounter - 0x200)<<std::endl;
    std::cout<<"memory[programCounter] is 0x"<<std::hex<<(int)memory[programCounter]<<std::endl;
    std::cout<<"memory[programCounter + 1] is 0x"<<std::hex<<(int)memory[programCounter + 1]<<std::endl;
    char readbuffer='0';
    std::cout<<"print memory? >>";
    std::cin>>readbuffer;
    if(readbuffer == '1' || readbuffer == 'y')
    {
        for(int i = 0; i<(int)sizeof(memory); i++)
            std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)memory[i] << " ";
        dumpRegisters();
    }
    //exit(0);
}

void chipdu::dumpRegisters()
{
    std::cout<<std::endl;
    std::cout<<"indexRegister is 0x"<<std::hex<<(int)indexRegister<<std::endl;
    std::cout<<"regsiters:"<<std::endl;
    for(int i = 0; i<(int)sizeof(registers); i++)
        std::cout<<"V"<<std::hex<<i<<": 0x"<<int(registers[i])<<std::endl;
    //std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)registers[i] << " ";
}
