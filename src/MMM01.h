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

#pragma  once

#include <cstdint>
#include <iostream>
#include <iomanip>
#include "Cartridge.h"
#include "MMM01.h"


/**
 * @brief Banked part of MMM01 type Cartridge.
 *
 * Handles bank switching for MMM01 type cartridges.
 *
 * MMM01 cartridges come up in unmapped mode which maps the top 32K of ROM to
 * 0-0x7fff.
 *
 * Writing to 0x4000-0x5fff writes the lower 2 bits into the upper 2 bits
 * of ROM bank address. In mode 1 the lower 4 bits are also put into the
 * RAM bank selection register.
 *
 * Writing to 0x6000-0x7fff writes the lower bit into the mode register.
 * Setting Mode to 1 enables RAM bank selecting, Mode 0 selects RAM bank
 * 0 only.
 *
 * @verbatim
 * unmapped Mapped
 *            22-15 14 13-00
 *            ^      ^   ^
 *            |      |   Lower address bits.
 *            |      +-- 1
 *            +--------- 1
 *
 * Mapped, unmultiplexed:
 *            22-21 20-19 18-14 13-00
 *            ^        ^   ^    ^
 *            |        |   |    Lower address bits.
 *            |        |   +--- ROM Bank Low (Map 00 to 01)
 *            |        +------- ROM Bank Mid
 *            +---------------- ROM Bank High
 *
 *
 * Mapped, Multiplexed Mode 0 & 1:
 *            22-21 20-19 18-14 13-00
 *            ^        ^   ^    ^
 *            |        |   |    Lower address bits.
 *            |        |   +--- ROM Bank Low
 *            |        +------- RAM Bank Low
 *            +---------------- ROM Bank High
 *
 * Mapped, Multiplexed Mode 1:
 *            22-21 20-19 18-14 13-00
 *            ^        ^   ^    ^
 *            |        |   |    Lower address bits.
 *            |        |   +--- And of ROM Bank Low and ROM Bank Mask
 *            |        +------- RAM Bank Low
 *            +---------------- ROM Bank High
 * @endverbatim
 *
 */
class Cartridge_MMM01_bank : public Cartridge_bank {
    bool       _mode;          /**< Cartridge addressing mode */
    bool       _mapped;        /**< Cartridge is in mapped mode */
    bool       _mode_lock;     /**< Prevent change of Mode bit */
    bool       _multiplex;     /**< Cartridge is in multiplexed mode */
public:
    uint32_t   top_bank;       /**< Top bank of ROM */
    uint32_t   rom_bank_reg;   /**< Current ROM Bank register */
    uint32_t   rom_bank_mask;  /**< Bank masking */
    uint32_t   ram_bank_reg;   /**< Current RAM Bank register */
    uint32_t   ram_bank_mask;  /**< Mask of which bits can be changed */

    Cartridge_MMM01_bank(uint8_t *data, size_t size) :
           Cartridge_bank(data, size) , _mode(false), _mapped(false),
           _mode_lock(false), _multiplex(false) {
        rom_bank_reg = 0;
        rom_bank_mask = 0;
        ram_bank_reg = 0;
        ram_bank_mask = 0x38 << 14;
        top_bank = (uint32_t)size - (32 * 1024);
    }

    Cartridge_MMM01_bank(const Cartridge_MMM01_bank &) = delete;

    Cartridge_MMM01_bank& operator=(const Cartridge_MMM01_bank &) = delete;

    /**
     * @brief Get current operating Mode of Cartridge mapper.
     *
     * @return current mapper mode.
     */
    virtual bool get_mode() const override {
        return _mode;
    }

    /**
     * @brief Set or reset mapped mode.
     *
     * @param mode New mapped mode.
     */
    virtual void set_mapped() {
         _mapped = true;
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
    virtual void read(uint8_t &data, uint16_t addr) const override;

    /**
     * @brief Handle writes to ROM.
     *
     * Handle mode selection and bank selection for MMM01 Cartridges.
     *
     * @param data Data to write to mode/bank register.
     * @param addr Address to write to.
     */
    virtual void write(uint8_t data, uint16_t addr) override;

};

/**
 * @brief Lower bank of MMM01 ROM Cartridge.
 *
 * The lower bank controls enabling of RAM and selection of the lower bank bits.
 *
 * Writing 0xxA to 0x0000-0x1fff enables the RAM. Writing anything else disables
 * RAM. Writing to 0x2000-0x3fff selects lower 5 bits (4 for MMM01M) cartridges.
 *
 * unmapped Mapped
 *            22-15 14 13-00
 *            ^      ^   ^
 *            |      |   Lower address bits.
 *            |      +-- 0
 *            +--------- 1
 *
 * Mapped, unmultiplexed:
 *            22-21 20-19 18-14 13-00
 *            ^        ^   ^    ^
 *            |        |   |    Lower address bits.
 *            |        |   +--- And of ROM Bank Low and ROM Bank Mask
 *            |        +------- RAM Bank Mid
 *            +---------------- ROM Bank High
 *
 * Mapped, Multiplexed Mode 0:
 *            22-21 20-19 18-14 13-00
 *            ^        ^   ^    ^
 *            |        |   |    Lower address bits.
 *            |        |   +--- And of ROM Bank Low and ROM Bank Mask
 *            |        +------- And of RAM Bank Low and RAM Bank Mask
 *            +---------------- ROM Bank High
 *
 * Mapped, Multiplexed Mode 1:
 *            22-21 20-19 18-14 13-00
 *            ^        ^   ^    ^
 *            |        |   |    Lower address bits.
 *            |        |   +--- And of ROM Bank Low and ROM Bank Mask
 *            |        +------- RAM Bank Low
 *            +---------------- ROM Bank High
 *
 *
 *
 */
class Cartridge_MMM01 : public Cartridge_ROM {
    Cartridge_MMM01_bank  *_rom_bank;        /**< Holds Banking object. */
    bool                  _mapped;           /**< Mapped mode */
    uint32_t              _mask;             /**< Mask for rom addresses */

public:
    Cartridge_MMM01(Memory *mem, uint8_t *data, size_t size, bool color) :
       Cartridge_ROM(mem, data, size, color), _mapped(false) {
        _rom_bank = new Cartridge_MMM01_bank(data, size);
        _mask = (uint32_t)(size - 1);
        std::cout << "MMM01";
    }

    Cartridge_MMM01(const Cartridge_MMM01 &) = delete;

    Cartridge_MMM01& operator=(const Cartridge_MMM01 &) = delete;

    ~Cartridge_MMM01() {
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
     * selects the lower 5 bits (or 4 for MMM01M) of ROM bank selection.
     */
    virtual void write(uint8_t data, uint16_t addr) override;

    /**
     * @brief Setup RAM for Cartridge.
     *
     * Use values in ROM to allocate space for on Cartridge RAM.
     * If there is already RAM make sure it is same size as
     * cartridge defined RAM.
     */
    virtual Cartridge_RAM *set_ram(int type, uint8_t *ram_data,
                             size_t ram_size)  override;

    /**
     * @brief Size of non-bank selection area.
     *
     * @return Always return 16K pages.
     */
    virtual size_t size() const override { return 64; }
};
