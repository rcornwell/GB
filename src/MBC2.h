/*
 * GB - Game Cartridge MBC2 cartridge controller
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
 * @brief RAM area for MBC2 Cartridges.
 *
 * MBC2 cartridges have small 512 nibble (4 bit) RAM inside the mapper.
 * There is no RAM bank selection.
 */
class Cartridge_MBC2_RAM : public Cartridge_RAM {
public:
    explicit Cartridge_MBC2_RAM(size_t size) : Cartridge_RAM(size) {
        _mask = 0x1ff;
    }

    /**
     * @brief Read RAM data.
     *
     * Only lower Nibble of RAM can be read. It is also repeated over
     * the entire address space.
     *
     * @param[out] data Data read from RAM.
     * @param[in] addr Address to read from.
     */
    virtual void read(uint8_t &data, uint16_t addr) const override {
       data = _data[addr & _mask] | 0xf0;
    }

    /**
     * @brief Write RAM data.
     *
     * Only lower Nibble of RAM can be writen. It is also repeated over
     * the entire address space.
     *
     * @param[in] data Data write to RAM.
     * @param[in] addr Address to read from.
     */
    virtual void write(uint8_t data, uint16_t addr) override {
       _data[addr & _mask] = data | 0xf0;
    }

    /**
     * @brief Size of RAM in 256 bytes.
     *
     * @return 32 for 8k of RAM.
     */
    virtual size_t size() const override {
       return 32;
    }

    /**
     * @brief Size of RAM to save to file.
     *
     * @return 512 bytes.
     */
    virtual size_t size_bytes() const {
       return 512;
    }
};

/**
 * @brief Lower bank of MBC2 ROM Cartridge.
 *
 * The lower bank controls enabling of RAM and selection of the lower bank bits.
 *
 * Writing 0xxA to 0x0000-0x1fff enables the RAM. Writing anything else disables RAM.
 * Writing to 0x2000-0x3fff selects lower 4 bank bits of cartridges.
 *
 */
class Cartridge_MBC2 : public Cartridge_ROM {
    Cartridge_bank  *_rom_bank;

public:
    Cartridge_MBC2(Memory *mem, uint8_t *data, size_t size, bool color) :
        Cartridge_ROM(mem, data, size, color) {
        _rom_bank = new Cartridge_bank(data, size);
       std::cout << "MBC2 Cartridge" << std::endl;
    }

    Cartridge_MBC2(const Cartridge_MBC2 &) = delete;

    Cartridge_MBC2& operator=(const Cartridge_MBC2 &) = delete;


    ~Cartridge_MBC2() {
         delete _rom_bank;
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
     * Writing to the lower bank either enables/disables to RAM or
     * selects the lower 4 bits of ROM bank selection.
     */
    virtual void write(uint8_t data, uint16_t addr) override {

        /* Preform bank switching */
        if ((addr & 0x100) == 0) {
                /* 0x0000 - 0x1eff */
                /* MBC2    enable ram. */
                /*       0xa    - enable. else disable */
                if ((data & 0xf) == 0xa) {
                    _mem->add_slice(_ram, 0xa000);
                } else {
                    _mem->add_slice_sz(&_empty, 0xa000, 32);
                }
        } else {   /* 0x2000 - 0x3fff */
                /* MBC2   select memory bank */
                /*       0 - F   bank for 0x4000-0x7fff */
                uint32_t new_bank = ((uint32_t)(data & 0xf)) << 14;
                if (new_bank == 0) {
                    new_bank = 0x4000;
                }
                new_bank &= _size - 1;
                _rom_bank->set_bank(new_bank);
         }
    }
};
