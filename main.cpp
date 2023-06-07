#define SDL_MAIN_HANDLED

#include <string>
#include <iostream>
#include <iomanip>  //remove this
#include <SDL2/SDL.h>
#include <chrono>
#include <thread>
#include "chipdu.h"

//used a lot for reference: https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

chipdu theChipdu;
float speedMultiplier = 1.0; //will speed up or slow down emulation. higher number = slower. lower number = faster.

/**
Original CHIP-8 keyboard layout is mapped to PC keyboard as follows:

    |1|2|3|C|  =>  |1|2|3|4|
    |4|5|6|D|  =>  |Q|W|E|R|
    |7|8|9|E|  =>  |A|S|D|F|
    |A|0|B|F|  =>  |Z|X|C|V|
**/

//  PC KEY      CHIP-8 KEY
uint8_t keymap[16] = {
    SDLK_x,     //"0"
    SDLK_1,     //"1"
    SDLK_2,     //"2"
    SDLK_3,     //"3"
    SDLK_q,     //"4"
    SDLK_w,     //"5"
    SDLK_e,     //"6"
    SDLK_a,     //"7"
    SDLK_s,     //"8"
    SDLK_d,     //"9"
    SDLK_z,     //"A"
    SDLK_c,     //"B"
    SDLK_4,     //"C"
    SDLK_r,     //"D"
    SDLK_f,     //"E"
    SDLK_v,     //"F"
};

int scale = 10; //trust me
int display_width = SCREEN_WIDTH * scale;
int display_height = SCREEN_HEIGHT * scale;

int main(int argc, char *argv[])
{
    //a good chunk of this is not mine but i swear i understand what it does
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) // INIT_VIDEO/INIT_AUDIO are defines. will both be true if we support video and audio.
    {
        std::cout<<"Failed to initialize SDL Library."<<std::endl;
        std::cout<<"SDL 2 error: "<< SDL_GetError() << std::endl; //SDL_GetError() is a function.
        return -1;
    }
    else
        ;
        //std::cout<<"SDL init good!"<<std::endl;

    // a window is the WHOLE window. x button/maxmize/minimize/title/all of it
    // this is the window we'll be rendering to
    SDL_Window* window = SDL_CreateWindow("Chipdu++",              //window title
                                        SDL_WINDOWPOS_CENTERED,   //window position x
                                        SDL_WINDOWPOS_CENTERED,   //window position y
                                        SCREEN_WIDTH*scale, SCREEN_HEIGHT*scale,    //x and y width and height (i have added an arbitrary scale value as well)
                                        0);                       //"Uint32 flags", none for now.
                                          //
    //check window
    if (!window)
    {
        std::cout<<"Failed to create window! \nSDL Error: "<<SDL_GetError()<<std::endl;
        return -1;
    }
    else
        ;
        //std::cout<<"Window init good!"<<std::endl;

    // Create renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);     //SDL_Window * window, int index, Uint32 flags
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH*scale, SCREEN_HEIGHT*scale);

    // Create texture that stores frame buffer
    SDL_Texture* sdlTexture = SDL_CreateTexture(renderer,
                                                SDL_PIXELFORMAT_ARGB8888,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                64, 32);
    //print emulator information on first open
    theChipdu.printEmulatorInformation();

    // Initialize the Chip8 system and load the game into the memory

    //if game file path is passed via command line, pass as filename.
    //if none is passed (NULL), loadProgram should pick up on this and invoke a filedialog

    bool firstLPressed = false;

    std::string filename = "";
    bool didLoadProgram;
    if (argc > 1)
    {
        filename = argv[1];
        didLoadProgram = theChipdu.loadProgram(filename);      //this will call init
        firstLPressed = true;   //if passed by CLI and load is successful, skip holding our user at the beginning. not tested lol
        //set to true regardless of if it actually worked or not so that we skip the loop, then if it failed it will catch that after the loop

    }

    SDL_Event e;
    while((firstLPressed == false))
    {
        while(SDL_PollEvent(&e))
        {
            if(e.type == SDL_KEYDOWN)
            {
                if(e.key.keysym.sym == SDLK_ESCAPE){exit(0);}  //if esc is pressed, exit

                if(e.key.keysym.sym == SDLK_l)  //hold us here at the beginning until the L key is pressed to LOAD a program
                {
                    firstLPressed = true;
                    didLoadProgram = theChipdu.loadProgram(filename);      //this will call init
                }
            }
            break;
        }
    }

    if(!didLoadProgram)
    {
        printf("Failed to load program! Stalling, then aborting...\n");
        std::this_thread::sleep_for(std::chrono::microseconds(5200));
        exit(-1);
    }

    printf("Program loaded! Starting emulation...\n");

    while(true)
    {
        if (e.type == SDL_QUIT) exit(0);
        theChipdu.doOneCycle();
        //theChipdu.debugRender();

        while (SDL_PollEvent(&e))   //check SDL events. this loop will end when there are no events to process.
        {
            if (e.type == SDL_QUIT){exit(0);}

            if (e.type == SDL_KEYDOWN)
            {
                if(e.key.keysym.sym == SDLK_ESCAPE){exit(0);}  //if esc is pressed, exit

                switch(e.key.keysym.sym)
                {
                case SDLK_LEFTBRACKET:
                    speedMultiplier *= 2.0;  //these look switched but i promise they're not
                    printf("speed: %.4f\n", 1/speedMultiplier);
                    break;

                case SDLK_RIGHTBRACKET:
                    speedMultiplier *= 0.5;
                    printf("speed: %.4f\n", 1/speedMultiplier);
                    break;
                }

                if(e.key.keysym.sym == SDLK_g){theChipdu.toggleDebugStuff();}

                for (int i = 0; i < 16; i++)
                {
                    if(e.key.keysym.sym == keymap[i])
                    {
                        theChipdu.keypad[i] = 1;
                    }
                }
            }

            if(e.type == SDL_KEYUP)
                for (int i = 0; i < 16; i++)
                {
                    if (e.key.keysym.sym == keymap[i])
                    {
                        theChipdu.keypad[i] = 0;
                    }
                }
        }
        //theChipdu.debugRender();
        if (theChipdu.drawFlag)
        {
            theChipdu.drawFlag = false;
            uint32_t pixels[32 * 64];
            for (int i = 0; i < 32 * 64; i++)
            {
                pixels[i] = (0x00FFFFFF * theChipdu.screen[i]) | 0xFF000000;
            }

            // Update SDL texture
            SDL_UpdateTexture(sdlTexture, NULL, pixels, 64 * sizeof(uint32_t));   //SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch
            // Clear screen and render
            SDL_RenderClear(renderer);  //clears whole renderer. SDL_Renderer *renderer
            SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);   //SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect* srcrect, const SDL_Rect *dstrect
                                                                //the source SDL_Rect structure or NULL for the entire texture -^^^-
                                                                //the destination SDL_Rect structure or NULL for the entire rendering target; -^^^- the texture will be stretched to fill the given rectangle
            //Update the screen with any rendering performed since the previous call.
            SDL_RenderPresent(renderer);    //SDL_Renderer *renderer
        }
        std::this_thread::sleep_for(std::chrono::microseconds((int)(1200*speedMultiplier)));   //we could make this adjustable in theory
    }

    return 0;
}

