/*
 * GB - Game Cartridge Logic.
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

#include "Cartridge.h"
#include "MBC1.h"
#include "MBC2.h"
#include "MBC3.h"
#include "MBC5.h"
#include "MMM01.h"

int rom_type[0x1f] = {
     /* 0 */   ROM,
     /* 1 */   MBC1,
     /* 2 */   MBC1+CRAM,
     /* 3 */   MBC1+CRAM+BAT,
     /* 4 */   ROM,  /* Not used */
     /* 5 */   MBC2,
     /* 6 */   MBC2+BAT,
     /* 7 */   ROM,  /* Not used */
     /* 8 */   ROM+CRAM,
     /* 9 */   ROM+CRAM+BAT,
     /* A */   ROM,  /* Not used */
     /* B */   MMM01,
     /* C */   MMM01+CRAM,
     /* D */   MMM01+CRAM+BAT,
     /* E */   ROM,  /* Not used */
     /* F */   MBC3+TIM+BAT,
     /* 10 */  MBC3+TIM+CRAM+BAT,
     /* 11 */  MBC3,
     /* 12 */  MBC3+CRAM,
     /* 13 */  MBC3+CRAM+BAT,
     /* 14 */  ROM,  /* Not used */
     /* 15 */  ROM,  /* Not used */
     /* 16 */  ROM,  /* Not used */
     /* 17 */  ROM,  /* Not used */
     /* 18 */  ROM,  /* Not used */
     /* 19 */  MBC5,
     /* 1A */  MBC5+CRAM,
     /* 1B */  MBC5+CRAM+BAT,
     /* 1C */  MBC5,
     /* 1D */  MBC5+CRAM,
     /* 1E */  MBC5+CRAM+BAT
};

/**
 * @brief Setup RAM for Cartridge.
 *
 * Use values in ROM to allocate space for on Cartridge RAM.
 * If there is already RAM make sure it is same size as
 * cartridge defined RAM.
 */
void Cartridge::set_ram() {
#define K  1024
     size_t     size;
     /* Compute size of RAM based on ROM data */
     switch (_data[0x149]) {
     default:
     case 0:    size = 0; break;
     case 1:    size = 2*K; break;
     case 2:    size = 8*K; break;
     case 3:    size = 32*K; break;
     case 4:    size = 128*K; break;
     }

     /* If we already have RAM data, try and use it */
     if (_ram_data != NULL) {
         if (size != _ram_size) {
             std::cerr << "Invalid Save file size: " << _ram_size <<
               " Cartridge says: " << size << std::endl;
             exit(1);
         }
         _ram = new Cartridge_RAM(_ram_data, _ram_size);
     } else {
         _ram = new Cartridge_RAM(size);
         _ram_size = size;
     }
     /* Connect ROM object to RAM to manage banks */
     _rom->set_ram(_ram);
}

/**
 * @brief Load pointer and size to backup RAM data.
 *
 * @param data Pointer to RAM data.
 * @param size Size of RAM data.
 */
void Cartridge::load_ram(uint8_t *data, size_t size) {
     _ram_data = data;
     _ram_size = size;
}

/**
 * @brief Return true if Cartridge type has battery backup.
 *
 * Look at Cartridge data and determine if a Battery should be present.
 * @return true if RAM should be saved.
 */
bool Cartridge::ram_battery() {
     uint8_t _type = _data[0x147];
     if (_type > 0x1e) {
         _type = 0;
     }
     int  type = rom_type[_type];
     return (type & BAT) != 0;
}

/**
 * @brief Check if header checksum is valid.
 *
 * Sum header data at a page of cartridge to determine if checksum is valid.
 */
bool Cartridge::header_checksum(int bank) {
     uint8_t   chk = 0;
     int       addr = (bank << 15);

     for (int i = 0x134; i <= 0x14c; i++) {
         chk = chk - _data[i + addr] - 1;
     }
     return (chk == _data[0x14d + addr]);
}

/**
 * @brief Give cartridge object a pointer to the contents of the ROM.
 *
 * Set pointers to ROM and size.
 * @param data Pointer to ROM data.
 * @param size Size of ROM in bytes.
 */
void Cartridge::set_rom(uint8_t *data, size_t size) {
     _data = data;
     _size = size;
     std::cout << "ROM = " << std::hex << (int)_data[0x147] << " SZ= " << (int)_data[0x148];
     std::cout << " RAM = " << (int)_data[0x149];
     uint8_t _type = _data[0x147];
     if (_type > 0x1e) {
         std::cerr << "Unknown ROM type" << std::endl;
         _type = 0;
     }
     int  type = rom_type[_type];
     std::cout << " type= " << std::hex << type;
     if (type & CRAM) {
        std::cout << " RAM";
     }
     if (type & BAT) {
        std::cout << " Battery";
     }
     if (type & TIM) {
        std::cout << " Timer";
     }
     std::cout << std::endl;
}

/**
 * @brief Setup Cartridge ROM and Bank switching.
 *
 * Look in the rom_type table to determine type and features of
 * ROM. Then create a object of the correct class to access the
 * ROM and handle bank switching as needed.
 */
void Cartridge::set_mem(Memory *mem) {
     _mem = mem;
     if (_data[0x147] > (sizeof(rom_type)/sizeof(int))) {
         _rom = new Cartridge_ROM(_mem, _data, _size);
     } else {
         int  type = rom_type[_data[0x147]];

         /* Special check for MMM01 cartridge */
         if ((type & 0xf) == MBC1 && _size > (64 * 1024)) {
             if (header_checksum((_size / (32 * 1024) - 1))) {
                 type = rom_type[_data[0x147 + (_size - (32 * 1024))]];
             }
         }
         switch (type & 0xf) {
         case ROM:
              _rom = new Cartridge_ROM(_mem, _data, _size);
              break;
         case MBC1:
              _rom = new Cartridge_MBC1(_mem, _data, _size);
              break;
         case MBC2:
              _rom = new Cartridge_MBC2(_mem, _data, _size);
              _ram = new Cartridge_MBC2_RAM(512);
              _rom->set_ram(_ram);
              break;
         case MBC3:
              _rom = new Cartridge_MBC3(_mem, _data, _size);
              break;
         case MBC5:
              _rom = new Cartridge_MBC5(_mem, _data, _size);
              break;
         case MMM01:
              _rom = new Cartridge_MMM01(_mem, _data, _size);
              break;
        }
        if (type & CRAM && _ram == NULL) {
            set_ram();
        }
     }
     _rom->map_cart();
}

