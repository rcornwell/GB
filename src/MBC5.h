/*
 * GB - Game Cartridge MBC5 cartridge controller
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

#pragma  once

#include <cstdint>
#include <iostream>
#include "Cartridge.h"

/**
 * @brief MBC5 bank ROM.
 *
 * MBC5 supports up to 512 banks of ROM, writing to the Bank controls
 * the upper 4 bits of RAM bank.
 */
class Cartridge_MBC5_bank : public Cartridge_bank {
public:
    Cartridge_MBC5_bank(uint8_t *data, size_t size) :
           Cartridge_bank(data, size) {
    }

    /**
     * @brief Handle write to ROM bank.
     *
     * Handle selection of RAM bank.
     *
     * @param data Data to write to ram bank.
     * @param addr Address to write to.
     */
    virtual void write(uint8_t data, uint16_t addr) override {
        /* Preform bank switching */
        switch (addr >> 13) {
        case 2:          /* 0x4000 - 0x5fff */
                /* MBC5   select ram bank */
                _ram_bank = ((uint32_t)(data) & 0xf) << 13;
                if (_ram) {
                    _ram->set_bank(_ram_bank);
                }
                break;
        case 3:          /* 0x6000 - 0x7fff */
                break;
        }
    }
};

/**
 * @brief Lower bank of MBC5 ROM cartridge.
 *
 * The lower bank controls enabling of RAM and the selection of the ROM bank.
 * Writing 0xxA to 0x0000-0x1fff enables the RAM, anything else disables the RAM.
 * Writing to 0x2000-0x2fff writes the lower 8 bits of the ROM bank address.
 * Writing to 0x3000-0x3fff writes the upper bit of ROM bank address.
 */
class Cartridge_MBC5 : public Cartridge_ROM {
    Cartridge_MBC5_bank  *_rom_bank;
    uint32_t              _bank;       /**< Local copy of bank number */
public:
    Cartridge_MBC5(Memory *mem, uint8_t *data, size_t size, bool color) :
       Cartridge_ROM(mem, data, size, color) {
        _rom_bank = new Cartridge_MBC5_bank(data, size);
        _bank = 0;
       std::cout << "MBC5 Cartridge " << (int)size;
    }

    Cartridge_MBC5(const Cartridge_MBC5 &) = delete;

    Cartridge_MBC5& operator=(const Cartridge_MBC5 &) = delete;

    ~Cartridge_MBC5() {
         delete _rom_bank;
    }

    /**
     * @brief Map Cartridge into Memory space.
     *
     * Map Cartridge ROM and RAM objects into memory space. Initially the RAM
     * is pointed to Empty until the RAM is enabled. If Boot ROM is enabled
     * (default) the boot ROM is mapped over the lower 256 bytes of ROM.
     */
    virtual void map_cart() override {
         _rom_bank->set_ram(_ram);
        /* Map ourselves in place */
        _mem->add_slice(this, 0);
        _mem->add_slice(_rom_bank, 0x4000);
        _mem->add_slice_sz(&_empty, 0xa000, 32);
        disable_rom(_rom_disable);
    }

    /**
     * @brief Handle writes to lower bank ROM.
     *
     * Enable/disable RAM or select ROM bank.
     */
    virtual void write(uint8_t data, uint16_t addr) override {
        uint32_t    new_bank;

        /* Preform bank switching */
        switch (addr >> 12) {
        case 0:          /* 0x0000 - 0x0fff */
        case 1:          /* 0x1000 - 0x1fff */
                /* MBC5    enable ram. */
                /*       0xa    - enable. else disable */
                if (_ram == NULL)
                    return;
                if ((data & 0xf) == 0xa) {
                   _mem->add_slice(_ram, 0xa000);
                } else {
                   _mem->add_slice_sz(&_empty, 0xa000, 32);
                }
                break;
        case 2:          /* 0x2000 - 0x2fff */
                /* MBC5   select memory bank */
                /*       0 - FF   bank for 0x4000-0x7fff */
                new_bank = ((uint32_t)(data & 0xff)) << 14;
                new_bank = (new_bank & 0x3fc000) | (_bank & 0x400000);
                new_bank &= _size - 1;
                _rom_bank->set_bank(new_bank);
                _bank = new_bank;
                break;
        case 3:          /* 0x3000 - 0x3fff */
                /* MBC5  select high bit of bank. */
                new_bank = ((uint32_t)(data & 0x1)) << 22;
                new_bank = (_bank & 0x3fc000) | (new_bank & 0x400000);
                new_bank &= _size - 1;
                _rom_bank->set_bank(new_bank);
                _bank = new_bank;
                break;
         }
    }

    /**
     * @brief Size of non-bank selection area.
     *
     * @return Always return 16K pages.
     */
    virtual size_t size() const override { return 64; }
};
