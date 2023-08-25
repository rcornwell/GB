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
#include <iomanip>
#include "Cartridge.h"

/**
 * @brief Banked part of MBC1 type Cartridge.
 *
 * Handles bank switching for MBC1 type cartridges.
 *
 * MBC1 type as a mode flag which indicates whether RAM is bank select-able
 * or not, other whether that function selects the High order bits of Bank
 * address.
 *
 * Writing to 0x4000-0x5fff writes the lower 2 bits into the upper 2 bits
 * of ROM bank address. In mode 1 the lower 4 bits are also put into the
 * RAM bank selection register.
 *
 * Writing to 0x6000-0x7fff writes the lower bit into the mode register.
 * Setting Mode to 1 enables RAM bank selecting, Mode 0 selects RAM bank
 * 0 only.
 */
class Cartridge_MBC1_bank : public Cartridge_bank {
    bool       _mode;         /**< Cartridge addressing mode */
    bool       _mbc1m;        /**< Is this a MBC1M cartridge */
public:
    Cartridge_MBC1_bank(uint8_t *data, size_t size) :
           Cartridge_bank(data, size) , _mode(false), _mbc1m(false) {
    }

    Cartridge_MBC1_bank(const Cartridge_MBC1_bank &) = delete;

    Cartridge_MBC1_bank& operator=(const Cartridge_MBC1_bank &) = delete;

    /**
     * @brief Get current operating Mode of Cartridge mapper.
     *
     * @return current mapper mode.
     */
    virtual bool get_mode() const override {
        return _mode;
    }

    /**
     * @brief Set cartridge to MBC1M type.
     */
    virtual void set_mbc1m() {
        _mbc1m = true;
    }

    /**
     * @brief Handle writes to ROM.
     *
     * Handle mode selection and bank selection for MBC1 Cartridges.
     *
     * @param data Data to write to mode/bank register.
     * @param addr Address to write to.
     */
    virtual void write(uint8_t data, uint16_t addr) override;
};

/**
 * @brief Lower bank of MBC1 ROM Cartridge.
 *
 * The lower bank controls enabling of RAM and selection of the lower bank bits.
 *
 * Writing 0xxA to 0x0000-0x1fff enables the RAM. Writing anything else disables
 * RAM. Writing to 0x2000-0x3fff selects lower 5 bits (4 for MBC1M) cartridges.
 *
 */
class Cartridge_MBC1 : public Cartridge_ROM {
    Cartridge_MBC1_bank  *_rom_bank;    /**< Holds Banking object. */
    bool                  _mbc1m;       /**< Cartridge is MBC1M type */

public:
    Cartridge_MBC1(Memory *mem, uint8_t *data, size_t size, bool color);

    Cartridge_MBC1(const Cartridge_MBC1 &) = delete;

    Cartridge_MBC1& operator=(const Cartridge_MBC1 &) = delete;

    ~Cartridge_MBC1() {
         delete _rom_bank;
    }

    /**
     * @brief Map Cartridge into Memory space.
     *
     * Map Cartridge ROM and RAM objects into memory space. Initially the RAM
     * is pointed to Empty until the RAM is enabled. If Boot ROM is enabled
     * (default) the boot ROM is mapped over the lower 256 bytes of ROM.
     */
    virtual void map_cart() override;

    /**
     * @brief Read data from cartridge ROM.
     *
     * If in Mode 1 bank 0 is bank select-able by upper bits of ROM.
     * Otherwise map to bank 0.
     *
     * @param[out] data Data read from ROM.
     * @param[in] addr Address to read from.
     */
    virtual void read(uint8_t &data, uint16_t addr) const override;

    /**
     * @brief Handle writes to lower bank.
     *
     * Writing to the lower bank either enables/disables to RAM or
     * selects the lower 5 bits (or 4 for MBC1M) of ROM bank selection.
     */
    virtual void write(uint8_t data, uint16_t addr) override;

    /**
     * @brief Size of non-bank selection area.
     *
     * @return Always return 16K pages.
     */
    virtual size_t size() const override { return 64; }
};
