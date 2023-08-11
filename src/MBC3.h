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

#pragma  once

#include <cstdint>
#include <iostream>
#include "Cartridge.h"

/**
 * @brief Banked part of MBC3 type Cartridge.
 *
 * Handles bank switching for MBC3 type cartridges.
 *
 * Writing to 0x4000-0x5fff write the lower 4 bits to RAM Bank selection
 * register.
 *
 * Writing to 0x6000-0x7fff Writing a 0 followed by a 1 will latch the
 * RTC registers.
 */
class Cartridge_MBC3_bank : public Cartridge_bank {
    bool       _latch;
public:
    Cartridge_MBC3_bank(uint8_t *data, size_t size) :
           Cartridge_bank(data, size) , _latch(false) {}

    /**
     * @brief Handle writes to ROM.
     *
     * @param data Data to write to bank register.
     * @param addr Address to write to.
     */
    virtual void write(uint8_t data, uint16_t addr) override {
        /* Preform bank switching */
        switch (addr >> 13) {
        case 2:          /* 0x4000 - 0x5fff */
                /* MBC3   select ram bank */
                _ram_bank = ((uint32_t)(data)) << 13;
                if (_ram) {
                    _ram->set_bank(_ram_bank);
                }
                break;
        case 3:          /* 0x6000 - 0x7fff */
                /* MBC3   select memory model */
                /*         0 select 16/8
                 *         1 select 4/32
                 */
                _latch = data & 1;
                break;
        }
    }
};

/**
 * @brief Lower bank of MBC3 ROM Cartridge.
 *
 * The lower bank controls enabling of RAM and selection of the lower bank bits.
 *
 * Writing 0xxA to 0x0000-0x1fff enables the RAM. Writing anything else disables RAM.
 * Writing to 0x2000-0x3fff selects lower 7 bits ROM bank cartridges.
 *
 */
class Cartridge_MBC3 : public Cartridge_ROM {
    Cartridge_bank  *_rom_bank;

public:
    Cartridge_MBC3(Memory *mem, uint8_t *data, size_t size, bool color) :
       Cartridge_ROM(mem, data, size, color) {
        _rom_bank = new Cartridge_MBC3_bank(data, size);
       std::cout << "MBC3 Cartridge" << std::endl;
    }

    Cartridge_MBC3(const Cartridge_MBC3 &) = delete;

    Cartridge_MBC3& operator=(const Cartridge_MBC3 &) = delete;

    ~Cartridge_MBC3() {
         delete _rom_bank;
    }

    /**
     * @brief Pass pointer to Caridge RAM so ROM can enable and select banks.
     *
     * Gives pointer to Cartrige RAM, this is passed to the Bank ROM.
     *
     * @param ram Pointer to Cartridge RAM.
     */
    virtual void set_ram(Cartridge_RAM *ram) override {
         _ram = ram;
         _rom_bank->set_ram(ram);
    }

    /**
     * @brief Map Cartridge into Memory space.
     *
     * Map Cartridge ROM and RAM objects into memory space. Initially the RAM
     * is pointed to Empty until the RAM is enabled. If Boot ROM is enabled (default)
     * the boot ROM is mapped over the lower 256 bytes of ROM.
     */
    virtual void map_cart() override {
        /* Map ourselves in place */
        _mem->add_slice(this, 0);
        _mem->add_slice(_rom_bank, 0x4000);
        _mem->add_slice_sz(&_empty, 0xa000, 32);
        disable_rom(_rom_disable);
    }

    /**
     * @brief Handle writes to lower bank.
     *
     * Write to lower bank either enable/disables to RAM or
     * Selects the lower 7 bits of the ROM bank.
     */
    virtual void write(uint8_t data, uint16_t addr) override {
        uint32_t    new_bank;

        /* Preform bank switching */
        switch (addr >> 13) {
        case 0:          /* 0x0000 - 0x1fff */
                /* MBC3    enable ram. */
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
                /* MBC3   select memory bank */
                /*       0 - 1F   bank for 0x4000-0x7fff */
                new_bank = ((uint32_t)(data & 0x7f)) << 14;
                if (new_bank == 0) {
                    new_bank = 0x4000;
                }
                new_bank &= _size - 1;
                _rom_bank->set_bank(new_bank);
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
