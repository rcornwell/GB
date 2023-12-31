/*
 * GB - Game Cartridge MBC1 cartridge controller
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
#include <iomanip>
#include "Cartridge.h"
#include "MBC1.h"

void Cartridge_MBC1_bank::write(uint8_t data, uint16_t addr) {
    uint32_t   new_bank;
    /* Preform bank switching */
    switch (addr >> 13) {
    case 2:          /* 0x4000 - 0x5fff */
            /* MBC1   select ram bank */
            /*  4/32 0-3      bank for 0xa000-0xbfff
             * 16/8  0-3      select high ROM address */
            if (_mbc1m) {
                new_bank = ((uint32_t)(data & 0x3)) << 18;
                new_bank |= _bank & 0x3c000;
            } else {
                new_bank = ((uint32_t)(data & 0x3)) << 19;
                new_bank |= _bank & 0x7c000;
            }
            _bank = new_bank & _mask;
            if (!_mbc1m && _mode) {
                _ram_bank = ((uint32_t)(data)) << 13;
                if (_ram) {
                    _ram->set_bank(_ram_bank);
                }
            }
            break;
    case 3:          /* 0x6000 - 0x7fff */
            /* MBC1   select memory model */
            /*         0 select 16/8
             *         1 select 4/32
             */
            _mode = data & 1;
            if (_mode) {
                if (_ram) {
                   _ram->set_bank(_ram_bank);
                }
            } else {
                if (_ram) {
                   _ram->set_bank(0);
                }
            }
            break;
    }
}

Cartridge_MBC1::Cartridge_MBC1(Memory *mem, uint8_t *data, size_t size, bool color) :
   Cartridge_ROM(mem, data, size, color) {
    _rom_bank = new Cartridge_MBC1_bank(data, size);

   /* Check if Cartridge is over 256K */
   if (size >= (256*1024)) {
      uint32_t   logo_test = 1 << 19;

      /* See if we can find a duplicate Logo in second bank */
      _mbc1m = true;   /* Set true for testing */
      for (int i = 0x104; i < 0x134; i++) {
          if (data[i] != data[logo_test+i]) {
              /* Mismatch, not MBC1M */
              _mbc1m = false;
              break;
          }
      }
   } else {
      _mbc1m = false;
   }
   if (_mbc1m) {
       std::cout << "MBC1M Cartridge";
       _rom_bank->set_mbc1m();
   } else {
       std::cout << "MBC1 Cartridge";
   }
}

void Cartridge_MBC1::map_cart() {
    /* Tell Bank about RAM */
     _rom_bank->set_ram(_ram);
    /* Map ourselves in place */
    _mem->add_slice(this, 0);
    _mem->add_slice(_rom_bank, 0x4000);
    /* Initially RAM is disabled */
    _mem->add_slice_sz(&_empty, 0xa000, 32);
    /* Initially the ROM is enabled */
    disable_rom(0);
}

void Cartridge_MBC1::read(uint8_t &data, uint16_t addr) const {
   if (_rom_bank->get_mode()) {
      uint32_t   bank = _rom_bank->get_bank();
      if (_mbc1m) {
          data = _data[(bank & 0x0c0000) + addr];
      } else {
          data = _data[(bank & 0x180000) + addr];
      }
   } else {
      data = _data[addr];
   }
}

void Cartridge_MBC1::write(uint8_t data, uint16_t addr) {
    uint32_t    new_bank;

    /* Preform bank switching */
    switch (addr >> 13) {
    case 0:          /* 0x0000 - 0x1fff */
            /* MBC1    enable ram. */
            /*       0xa    - enable. else disable */
            if (_ram == NULL)
                return;
            if ((data & 0xf) == 0xa) {
               _mem->add_slice(_ram, 0xa000);
            } else {
               _mem->add_slice_sz(&_empty, 0xa000, 32);
            }
            break;
    case 1:          /* 0x2000 - 0x3fff */
            /* MBC1   select memory bank */
            /*       0 - 1F   bank for 0x4000-0x7fff */
            if (_mbc1m) {
                new_bank = ((uint32_t)(data & 0xf)) << 14;
                new_bank |= _rom_bank->get_bank() & 0xc0000;
            } else {
                new_bank = ((uint32_t)(data & 0x1f)) << 14;
                new_bank |= _rom_bank->get_bank() & 0x180000;
            }
            if ((data & 0x1f) == 0) {
                new_bank = 0x4000;
            }
            new_bank &= _size - 1;
            _rom_bank->set_bank(new_bank);
            break;
     }
}

