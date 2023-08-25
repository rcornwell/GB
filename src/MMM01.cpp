/*
 * GB - Game Cartridge MMM01 cartridge controller
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
#include "MMM01.h"


/**
 * @brief Read data from cartridge ROM.
 *
 * If in Mode 1 bank 0 is bank select-able by upper bits of ROM.
 * Otherwise map to bank 0.
 *
 * @param[out] data Data read from ROM.
 * @param[in] addr Address to read from.
 */
void Cartridge_MMM01_bank::read(uint8_t &data, uint16_t addr) const {
    uint32_t bank;
    if (_mapped) {
        bank = _bank;
    } else {
        bank = top_bank | 0x4000;
    }
    data = _data[(bank | (addr & 0x3fff)) & _mask];
}

/**
 * @brief Handle writes to ROM.
 *
 * Handle mode selection and bank selection for MMM01 Cartridges.
 *
 * @param data Data to write to mode/bank register.
 * @param addr Address to write to.
 */
void Cartridge_MMM01_bank::write(uint8_t data, uint16_t addr) {
    uint32_t   new_bank;
    uint32_t   new_ram_bank;
    /* Preform bank switching */
    switch (addr >> 13) {
    case 2:          /* 0x4000 - 0x5fff */
            /* MMM01   select ram bank */
            /*  4/32 0-3      bank for 0xa000-0xbfff
             * 16/8  0-3      select high ROM address */
            if (!_mapped) {
                ram_bank_reg = (uint32_t)(data & 0xf) << 13;
                _mode_lock = ((data & 0x40) != 0);
            } else {
                new_bank = (uint32_t)(data & 0x3) << 13;
                ram_bank_reg = (ram_bank_reg & ram_bank_mask) |
                                 (new_bank & (~ram_bank_mask));
            }
            /* Always set RAM Bank */
            if (_multiplex) {
                new_ram_bank = ((rom_bank_reg >> 5) & 0x6000) |
                                 (ram_bank_reg & 0x18000);
                new_bank = (ram_bank_reg & 3) << 6;
                new_bank |= (rom_bank_reg & 0x33e000);
            } else {
               new_ram_bank = ram_bank_reg;
               new_bank = rom_bank_reg;
            }
            set_bank(new_bank & _mask);
            if (_ram) {
                _ram->set_bank(new_ram_bank);
            }
            break;
    case 3:          /* 0x6000 - 0x7fff */
            /* MMM01   select memory model */
            /*         0 select 16/8
             *         1 select 4/32
             *     5-2   Set ROM Bank mask.
             *     6     Set multiplex mode.
             */

            /* Can only be changed in unmapped mode */
            if (!_mapped) {
                /* Set Mid and High bits to be unchangeable */
                rom_bank_mask = (0x3c0 | (data & 0x3c)) << 14;
                _multiplex = (data & 0x40) != 0;
            }
            /* If locked out can't change mode */
            if (!_mode_lock) {
                _mode = data & 1;
            }
            /* Always set RAM Bank */
            if (_multiplex) {
               new_ram_bank = ((rom_bank_reg >> 5) & 0x6000) |
                                 (ram_bank_reg & 0x18000);
            } else {
               new_ram_bank = ram_bank_reg;
            }
            if (_ram) {
               _ram->set_bank(new_ram_bank);
            }
            break;
    }
}


/**
 * @brief Map Cartridge into Memory space.
 *
 * Map Cartridge ROM and RAM objects into memory space. Initially the RAM
 * is pointed to Empty until the RAM is enabled. If Boot ROM is enabled
 * (default) the boot ROM is mapped over the lower 256 bytes of ROM.
 */
void Cartridge_MMM01::map_cart() {
     _rom_bank->set_ram(_ram);
    /* Map ourselves in place */
    _mem->add_slice(this, 0);
    _mem->add_slice(_rom_bank, 0x4000);
    _mem->add_slice_sz(&_empty, 0xa000, 32);
    disable_rom(_rom_disable);
}

/**
 * @brief Read data from cartridge ROM.
 *
 * If in Mode 1 bank 0 is bank select-able by upper bits of ROM.
 * Otherwise map to bank 0.
 *
 * @param[out] data Data read from ROM.
 * @param[in] addr Address to read from.
 */
void Cartridge_MMM01::read(uint8_t &data, uint16_t addr) const {
   uint32_t bank;
   if (_mapped) {
       bank = _rom_bank->get_bank() & _rom_bank->rom_bank_mask;
   } else {
       bank = _rom_bank->top_bank;
   }
   data = _data[(bank | (addr & 0x3fff)) & _mask];
}

/**
 * @brief Handle writes to lower bank.
 *
 * Writing to the lower bank either enables/disables to RAM or
 * selects the lower 5 bits (or 4 for MMM01M) of ROM bank selection.
 */
void Cartridge_MMM01::write(uint8_t data, uint16_t addr) {
    uint32_t    new_bank;

    /* Preform bank switching */
    switch (addr >> 13) {
    case 0:          /* 0x0000 - 0x1fff */
            /* MMM01    enable ram. */
            /*       0xa    - enable. else disable */
            if (!_mapped) {
                _rom_bank->ram_bank_mask = (0xc | ((data >> 4) & 0x3)) << 14;
                if ((data & 0x40) != 0) {
                    _mapped = true;
                    _rom_bank->set_mapped();
                }
            }
            if (_ram == NULL)
                return;
            if ((data & 0xf) == 0xa) {
               _mem->add_slice(_ram, 0xa000);
            } else {
               _mem->add_slice_sz(&_empty, 0xa000, 32);
            }
            break;
    case 1:          /* 0x2000 - 0x3fff */
            /* MMM01   select memory bank */
            /* Bank mid = 0x60, Low = 0x1f */
            /* mask = 0x38 */
            /* Rom high = 0x70 mbc1 disable, High = 30, Ram = 0 */
            /*       0 - 7F   bank for 0x4000-0x7fff */
            if (!_mapped) {
                new_bank = ((uint32_t)(data & 0x7f)) << 14;
                if ((data & 0x1f) == 0) {
                    new_bank |= 0x4000;
                }
                /* Merge current high order bits with new bits */
                new_bank = (_rom_bank->rom_bank_reg & 0x600000) |
                                          (new_bank & 0x1fc000);
            } else {
                new_bank = ((uint32_t)(data & 0x1f)) << 14;
                if (new_bank == 0) {
                    new_bank = 0x4000;
                }
                new_bank = (_rom_bank->rom_bank_reg & _rom_bank->rom_bank_mask) |
                            (new_bank & (~_rom_bank->rom_bank_mask));
            }
            _rom_bank->rom_bank_reg = new_bank;
            _rom_bank->set_bank(new_bank);
            break;
     }
}

/**
 * @brief Setup RAM for Cartridge.
 *
 * Use values in ROM to allocate space for on Cartridge RAM.
 * If there is already RAM make sure it is same size as
 * cartridge defined RAM.
 */
Cartridge_RAM *Cartridge_MMM01::set_ram([[maybe_unused]]int type,
                uint8_t *ram_data, size_t ram_size)  {
#define K  1024
     size_t     sze;

     /* Compute size of RAM based on ROM data */
     switch (_data[_rom_bank->top_bank + 0x149]) {
     default:
     case 0:    sze = 0; break;
     case 1:    sze = 2*K; break;
     case 2:    sze = 8*K; break;
     case 3:    sze = 32*K; break;
     case 4:    sze = 128*K; break;
     }

     std::cout << " Ram: " << std::dec << (int)sze << std::endl;
     /* If size of RAM is zero, just return. */
     if (sze == 0) {
         return _ram;
     }
     /* If we already have RAM data, try and use it */
     if (ram_data != NULL) {
         if (sze != ram_size) {
             std::cerr << "Invalid Save file size: " << ram_size <<
               " Cartridge says: " << sze << std::endl;
             exit(1);
         }
         _ram = new Cartridge_RAM(ram_data, ram_size);
     } else {
         _ram = new Cartridge_RAM(sze);
     }
     return _ram;
}
