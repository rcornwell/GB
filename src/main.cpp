/*
 * GB - Main start-up file.
 *
 * Author:      Richard Cornwell (rich@sky-visions.com)
 * Copyright 2023, Richard Cornwell
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include <cstdint>
#include <iostream>
#include <fstream>
#include <SDL.h>
#ifndef _WIN32
#include <SDL_main.h>
#endif
#include <SDL_mixer.h>
#include <SDL_image.h>

#ifndef _WIN32
#include "config.h"
#else
#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#endif
#include "System.h"
#include "Cpu.h"
#include "Joypad.h"
#include "Cartridge.h"
const
#include "../image/icon.xpm"

/**
 * @brief Main interface between emulator and SDL.
 *
 * This file holds the interface between the emulator and SDL library.
 * This is not a class since these functions interface with C libraries.
 */

SDL_Window       *window;
SDL_Surface      *icon;
SDL_Renderer     *render;
SDL_Color         palette[128];
SDL_AudioDeviceID audio_device;
SDL_AudioSpec     request;
SDL_AudioSpec     obtained;

int8_t            audio_buffer[2048]; /**< Buffer of generated samples */
int               audio_pos;          /**< Position to write audio samples */

#define CYCLES_PER_SCREEN 17556
#define FRAME_TIME        16.650f

int               scale;              /**< Screen scaling */
bool              POWER;              /**< Game boy power state */
bool              trace_flag;         /**< Trace instruction execution */
bool              color;              /**< Color Game Boy */

std::string       rom_name;           /**< ROM name being loaded */
std::string       sav_name;           /**< Backup RAM Memory */

std::string       host;
int               port;

Joypad            *joy;               /**< Pointer to Joypad device */
Cpu               *cpu;               /**< Pointer to CPU */

/**
 * @brief  main, entry to system.
 *
 * Scan arguments looking for scale and cartridge file names to load.
 */
int main(int argc, char **argv)
{
     Cartridge     *cart;        /* Cartridge object */
     int           i;            /* Temp */
     uint8_t       *rom = NULL;  /* Cartridge binary data */
     uint8_t       *ram = NULL;  /* Save RAM */
     int           help = false;

     /* Print banner out */
     std::cout << "Game Boy Emulator (" << VERSION_MAJOR << "."
               << VERSION_MINOR << ")" << std::endl;
     scale = 4;
     POWER = 1;
     trace_flag = false;
     color = false;

     /* Scan for arguments */
     for (i = 1; i < argc; i++) {
         if (argv[i][0] == '-') {
             char     *p = argv[i];

             while (*++p != '\0') {
                 switch(*p) {
                 case '1':
                 case '2':
                 case '3':
                 case '4':
                 case '5':
                 case '6':
                 case '7':
                 case '8':
                 case '9':
                           scale = *p - '0';
                           break;
                 case 'c': color = true; break;
                 case 'b': color = false; break;
                 case 't': trace_flag = true; break;
                 case 'p': port = atoi(argv[++i]); break;
                 case 'h': host = argv[++i]; break;
                 default:  help = true; break;
                 }
             }
         } else {
             /* First name is rom name */
             if (rom_name.empty()) {
                 rom_name = argv[i];
             } else if (sav_name.empty()) {
                 std::cerr << "To many arguments." << std::endl;
                 exit(1);
             } else {
                 sav_name = argv[i];
             }
         }
     }

     if (rom_name.empty()) {
        std::cout << "Missing rom!" << std::endl;
        help = true;
     }

     if (help) {
        std::cout << "Print help here" << std::endl;
        exit(1);
     }

     std::cout << "Scale = " << scale << " Rom name = " << rom_name;
     if (!sav_name.empty()) {
        std::cout << " Battery RAM = " << sav_name;
     }
     std::cout << std::endl;

     /* Open up and read in ROM file */
     std::ifstream inputFile(rom_name, std::ios::binary | std::ios::in);

     if (!inputFile) {
        std::cerr << "Failed to open file: " << rom_name << std::endl;
        return 1;
     }

     inputFile.seekg(0, inputFile.end);
     size_t rom_size = inputFile.tellg();
     inputFile.seekg(0, inputFile.beg);

     rom = new uint8_t[rom_size];

     inputFile.read((char *)rom, rom_size);
     inputFile.close();

     std::cerr << "Read " << (int)rom_size << " bytes from " <<
                rom_name << std::endl;

     /* Create Cartridge object */
     cart = new Cartridge(rom, rom_size, color);

     /* Check if no save name give. */
     if (sav_name.empty()) {
         sav_name = rom_name;
         std::string::size_type sz = sav_name.rfind('.', sav_name.length());
         if (sz != std::string::npos) {
             sav_name.replace(sz+1, 3, "sav");
         }
     }

     /* Try to read in the RAM file. */
     std::ifstream ramFile(sav_name, std::ios::binary | std::ios::in);

     if (ramFile) {
         ramFile.seekg(0, ramFile.end);
         size_t ram_sz = ramFile.tellg();
         ramFile.seekg(0, ramFile.beg);

         ram = new uint8_t[ram_sz];

         ramFile.read((char *)ram, ram_sz);
         ramFile.close();
         std::cerr << "Read " << (int)ram_sz << " bytes from " <<
                    sav_name << std::endl;

         cart->load_ram(ram, ram_sz);
     }

     /* Create an emulation system. */
     cpu = new Cpu(cart, color);

     /* Set up SDL windows */
     init_window();

     /* Run the simulation */
     run_sim();

     /* When finished check if Cartridge was battery backed up */
     if (cart->ram_battery()) {
         uint8_t        *ram_data = cart->ram_data();
         size_t          ram_size = cart->ram_size();
         std::fstream    save_file;

         if (ram_data == NULL) {
             std::cerr << "No data to save" << std::endl;
         } else {
             std::cout << "Save file: " << ram_size << std::endl;
             save_file.open(sav_name, std::ios_base::out|std::ios_base::binary);
             if (!save_file.is_open()) {
                 std::cerr << "Unable to save RAM to: " << sav_name
                                 << std::endl;
             } else {
                 save_file.write((char *)ram_data, ram_size);
                 save_file.close();
             }
         }
     }

     /* Clean up house */
     delete cpu;
     delete cart;
     delete[] rom;
     delete[] ram;
     return 0;
}

/**
 * @brief Initialize SDL window and sound system.
 *
 * Create an SDL window and audio device.
 */
void init_window()
{

    /* Start SDL */
    SDL_Init( SDL_INIT_EVERYTHING );

    window = SDL_CreateWindow("Game Boy", SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED,
                           160*scale, 144*scale, SDL_WINDOW_RESIZABLE );
    /* Create icon for display */
    icon = IMG_ReadXPMFromArray((char **)icon_image);
    SDL_SetWindowIcon(window, icon);

    /* Request audio playback */
    request.freq = 32768;
    request.format = AUDIO_S8;
    request.channels = 2;
    request.samples = 2048;
    audio_device = SDL_OpenAudioDevice(NULL, 0, &request, &obtained, 0);
    if (audio_device == 0) {
       std::cerr << "Failed to get audio device" << std::endl;
       exit(1);
    }
}

/**
 * @brief Called before start of screen display.
 *
 * Clear display for new draw.
 */
void init_screen()
{
}


/**
 * @brief Called after screen drawing complete.
 *
 * Present screen to SDL for display.
 */
void
draw_screen()
{
    SDL_RenderPresent( render );
    /* Clear display */
    SDL_SetRenderDrawColor( render, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear( render);
}

SDL_Color base_color[4] = {
     { 0x9d, 0xbc, 0x0f, 0xff },
     { 0x7b, 0xac, 0x0f, 0xff },
     { 0x30, 0x62, 0x30, 0xff },
     { 0x0f, 0x38, 0x0f, 0xff }
};

/**
 * @brief Set SDL palette based on packed data.
 *
 * Set drawing colors based on 4 2 bit pixel colors given.
 */
void
set_palette(int num, uint8_t data)
{
     int i;

     for (i = 0; i < 4; i++) {
         int c = data & 03;

         palette[num+i] = base_color[c];
         data >>= 2;
     }
}

/**
 * @brief Set SDL palette based on packed color data.
 *
 * Set drawing colors based on
 */
void
set_palette_col(int num, uint8_t data_l, uint8_t data_h)
{
     palette[num].r = (data_l & 0x1f) << 3;
     palette[num].g = ((data_l & 0xe0) >> 3) | ((data_h & 0x3) << 6);
     palette[num].b = ((data_h & 0x7c) << 1);
     palette[num].a = 0xff;
}

/**
 * @brief Draw a pixel.
 *
 * Draw a rectangle of scale size and given color.
 */
void
draw_pixel(uint8_t pix, int row, int col)
{
     SDL_Rect    rect;

     rect.y = row * scale;
     rect.x = col * scale;
     rect.h = scale;
     rect.w = scale;
     SDL_SetRenderDrawColor(render, palette[pix].r,
                                    palette[pix].g,
                                    palette[pix].b,
                                    palette[pix].a);
     SDL_RenderFillRect(render, &rect);
}

/**
 * @brief Let SDL know where to send keypress/release events.
 */

void
set_joypad(Joypad *_joy)
{
     joy = _joy;
}

/**
 * @brief Send out one audio sample.
 *
 * Send out a stereo pair of audio samples to let SDL grab them later.
 */
void
audio_output(int8_t right, int8_t left)
{
   audio_buffer[audio_pos++] = right;
   audio_buffer[audio_pos++] = left;
}

/**
 * @brief Main simulation loop.
 *
 * Initialize the audio device, create render space, then loop until
 * POWER becomes false; process any events, then run CPU simulation
 * for one display frame, queue up any audio data produced. Then wait
 * for about 16.8MS. Next determine exactly how much time has passed
 * and adjust next cycle length to keep at fixed rate.
 */
void
run_sim()
{
    SDL_Event event;
    float  time_left = 0.0f;

    /* Initialize SDL for display */
    POWER = true;
    SDL_PauseAudioDevice(audio_device, 0);
    render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor( render, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear( render);
    SDL_RenderPresent( render );
    if (trace_flag) {
       cpu->run();
    }
    while(POWER) {
       /* Process events */
       uint64_t     start_time = SDL_GetPerformanceCounter();
       while(SDL_PollEvent(&event)) {
          switch(event.type) {
          case SDL_MOUSEBUTTONDOWN:
               break;
          case SDL_MOUSEBUTTONUP:
               break;
          case SDL_KEYUP:
                /* Cursors direction buttons.
                 *   X = A button.
                 *   Z = B button.
                 * shift = Select.
                 * enter = Start.
                 */
                 switch(event.key.keysym.scancode) {
                 case SDL_SCANCODE_X:
                         if (joy) {
                             joy->release_button(ABUT);
                         }
                         break;

                 case SDL_SCANCODE_Z:
                         if (joy) {
                             joy->release_button(BBUT);
                         }
                         break;

                 case SDL_SCANCODE_RETURN:
                         if (joy) {
                             joy->release_button(START);
                         }
                         break;

                 case SDL_SCANCODE_LSHIFT:
                         if (joy) {
                             joy->release_button(SELECT);
                         }
                         break;

                 case SDL_SCANCODE_RIGHT:
                         if (joy) {
                             joy->release_button(RIGHT);
                         }
                         break;

                 case SDL_SCANCODE_LEFT:
                         if (joy) {
                             joy->release_button(LEFT);
                         }
                         break;

                 case SDL_SCANCODE_UP:
                         if (joy) {
                             joy->release_button(UP);
                         }
                         break;

                 case SDL_SCANCODE_DOWN:
                         if (joy) {
                             joy->release_button(DOWN);
                         }
                         break;

                 case SDL_SCANCODE_Q:
                         POWER = false;
                         break;
                 default:
                         break;
                 }
                 break;

          case SDL_KEYDOWN:
                /* Cursors direction buttons.
                 *   X = A button.
                 *   Z = B button.
                 * shift = Select.
                 * enter = Start.
                 */
                 switch(event.key.keysym.scancode) {
                 case SDL_SCANCODE_X:
                         if (joy) {
                             joy->press_button(ABUT);
                         }
                         break;

                 case SDL_SCANCODE_Z:
                         if (joy) {
                             joy->press_button(BBUT);
                         }
                         break;

                 case SDL_SCANCODE_RETURN:
                         if (joy) {
                             joy->press_button(START);
                         }
                         break;

                 case SDL_SCANCODE_LSHIFT:
                         if (joy) {
                             joy->press_button(SELECT);
                         }
                         break;

                 case SDL_SCANCODE_RIGHT:
                         if (joy) {
                             joy->press_button(RIGHT);
                         }
                         break;

                 case SDL_SCANCODE_LEFT:
                         if (joy) {
                             joy->press_button(LEFT);
                         }
                         break;

                 case SDL_SCANCODE_UP:
                         if (joy) {
                             joy->press_button(UP);
                         }
                         break;

                 case SDL_SCANCODE_DOWN:
                         if (joy) {
                             joy->press_button(DOWN);
                         }
                         break;

                 case SDL_SCANCODE_Q:
                         POWER = false;
                         break;
                 default:
                         break;
                 }
                 break;
          case SDL_WINDOWEVENT:
                 switch (event.window.event) {
                 case SDL_WINDOWEVENT_CLOSE:
                      break;
                 }
                 break;
          case SDL_QUIT:
                 POWER = false;
                 break;
          default:
                 break;
          }
       }
       /* Run for 16.742ms */
       while(cpu->get_cycles() < CYCLES_PER_SCREEN) {
          cpu->step();
          if (trace_flag && !cpu->halted) {
              cpu->trace();
          }
       }
       /* Tell CPU how many major cycles it should have run */
       cpu->reset_cycles(CYCLES_PER_SCREEN);

       /* If CPU running, push out audio samples */
       if (cpu->running && !trace_flag)
           SDL_QueueAudio(audio_device, (void *)audio_buffer, audio_pos);
       audio_pos = 0;

       /* Compute how long to wait for before next screen */
       uint64_t end_time = SDL_GetPerformanceCounter();
       float elapsedMS = (end_time - start_time) /
                (float)SDL_GetPerformanceFrequency() * 1000.0f;

       /* Add in previous frame remainder */
       elapsedMS += time_left;
       if (elapsedMS < FRAME_TIME) {
           cpu->run();
           SDL_Delay((uint32_t)floor(FRAME_TIME - elapsedMS));
       }

       /* Compute amount of time delay actually waited for */
       uint64_t frame_time = SDL_GetPerformanceCounter();
       float frameMS = (frame_time - start_time) /
                (float)SDL_GetPerformanceFrequency() * 1000.0f;

       /* Adjust next frame to be correct */
       time_left = frameMS - FRAME_TIME;
    }

    /* Clean up house */
    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
}

