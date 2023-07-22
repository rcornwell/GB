/*
 * GB - Test set main startup file.
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

#include <iostream>
#include <fstream>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "Cpu.h"
#include "Joypad.h"
#include "Memory.h"
#include "System.h"
#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"

using namespace std;

bool        trace_flag = false;
Joypad      *joy;

uint8_t    lcd_panel[160][144];

void init_window()
{
}

void init_screen()
{
}

void draw_screen()
{
}

void set_joypad(Joypad *_joy)
{
    joy = _joy;
}

void set_palette(int num, uint8_t data)
{
}

void draw_pixel(uint8_t pix, int row, int col)
{
    lcd_panel[row][col] = pix;
}

void run_sim()
{
}

void audio_output(uint8_t right, uint8_t left)
{
}


extern uint8_t  cpu_inst[];
extern size_t   cpu_inst_sz;
extern uint8_t  instr_timing[];
extern size_t   instr_timing_sz;
extern uint8_t  mem_timing[];
extern size_t   mem_timing_sz;

TEST_GROUP(CPU)
{
};

TEST(CPU, memtiming)
{
     Cartridge     cart;

     cart.set_rom(&mem_timing[0], mem_timing_sz);

     Cpu          *cpu = new Cpu(&cart);

     cpu->run();

     while (cpu->pc != 0x6f1) {
          cpu->step();
     }
     delete cpu;
}

TEST(CPU, timing)
{
     Cartridge     cart;

     cart.set_rom(&instr_timing[0], instr_timing_sz);

     Cpu          *cpu = new Cpu(&cart);

     cpu->run();

     while (cpu->pc != 0xc8b0) {
          cpu->step();
     }
     delete cpu;
}

TEST(CPU, instr)
{
    uint64_t    tim = 0;
    uint64_t    n_inst = 0;

    Cartridge     cart;
    cart.set_rom(&cpu_inst[0], cpu_inst_sz);
    Cpu          *cpu = new Cpu(&cart);
    cpu->run();
    auto start = chrono::high_resolution_clock::now();
    while (cpu->pc != 0x6f1) {
         cpu->step();
         n_inst++;
    }
    tim = cpu->get_cycles();
    cout << endl;
    auto end = chrono::high_resolution_clock::now();
    cout << "Simulated time: " << dec << tim << endl;
    cout << "Excuted: " << n_inst << endl;
    auto s = chrono::duration_cast<chrono::seconds>(end - start);
    cout << "Run time: " << s.count() << " seconds" << endl;
    auto ctim = chrono::duration_cast<chrono::nanoseconds>(end - start);
    cout << "Time: " << ctim.count() << " ns" << endl;
    int cy  = ((ctim.count()) / (tim));
    cout << "Cycle time: " << (cy / 10) << "." << (cy % 10) << " ns" << endl;
    cout << "Instruct time: " << (ctim.count() / n_inst) << " ns" << endl;

     delete cpu;
}

// run all tests
int main(int argc, char **argv)
{
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
