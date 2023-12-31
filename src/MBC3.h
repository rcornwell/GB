/*
 * GB - Game Cartridge MBC3 cartridge controller
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

class Cartridge_MBC3;

/**
 * @brief Holds cartridge ram if any.
 *
 * Cartridge ram for MBC3 cartridges with RTC option.
 * multiple banks of RAM, this is controlled by mapper hardware. Similar
 * to RAM object, but supports paging. Cartridges can have anywhere from
 * 0 bytes of memory to 32Kbytes of memory.
 */
class Cartridge_MBC3_RAM : public Cartridge_RAM {
    bool        _latched;       /**< Current time is latched */
    uint16_t    _rtc_base;      /**< Base address of RTC */
public:

    /**
     * @brief Create Cartridge RAM space.
     *
     * Create RAM space for a Cartridge, size specifies power
     * of 2 number of bytes to allocation. Also mask off any
     * un-accessible bits of bank select if RAM is too small to
     * have banks. This makes RAM bank bits over size to be
     * effectively ignored.
     *
     * @param size Amount in bytes of RAM to allocate.
     */
    explicit Cartridge_MBC3_RAM(size_t size) : Cartridge_RAM() {
        _data = new uint8_t [size + 48];
        _mask = 0x1fff;
        _rtc_base = (uint16_t)(size & 0xffff);
        _size = size;
        _bank_mask = 0xf;
        _bank = 0;
        _need_delete = true;
        _latched = false;
    }

    /**
     * @brief Create a Cartridge RAM from file.
     *
     * Create RAM space for a Cartridge, size is number of bytes
     * in file, and data points to existing data.
     *
     * @param data Pointer to data to access.
     * @param size Size of data in bytes.
     */
    explicit Cartridge_MBC3_RAM(uint8_t *data, size_t size) : Cartridge_RAM() {
        _data = data;
        _mask = 0x1fff;
        _rtc_base = (uint16_t)(size & 0xffff);
        _size = size;
        _bank = 0;
        _bank_mask = 0xf;
        _need_delete = false;
        _latched = false;
    }

    /**
     * @brief Routine to read from memory.
     *
     * Return the value based on the mask to select range of access.
     *
     * @param[out] data Data read from memory.
     * @param[in] addr Address of memory to read.
     */
    virtual void read(uint8_t &data, uint16_t addr) const override;

    /**
     * @brief Routine to write to memory.
     *
     * Set memory at address to data, based on mask.
     * @param[in] data Data to write to memory.
     * @param[in] addr Address of memory to write.
     */
    virtual void write(uint8_t data, uint16_t addr) override;

    /**
     * @brief Size of RAM in 256 bytes.
     *
     * @return 32 for 8k of RAM.
     */
    virtual size_t size() const override {
       return 32;
    }

    /**
     * @brief Return RAM data pointer.
     *
     * Update the time value in the end of memory before returning pointer.
     *
     * @return pointer to ram data.
     */
    virtual uint8_t *ram_data() override;

    /**
     * @brief Return RAM size of RAM in bytes.
     *
     * @return size of RAM in bytes.
     */
    virtual size_t ram_size() override {
           return _size + 48;
    }

    /**
     * @brief Latch time value.
     *
     */
    void latch(uint8_t data);

    /**
     * @brief Tick callback function.
     *
     * Called once per second to update the time value.
     */
    virtual void tick() override;

    /**
     * @brief Update time when loading cartridge from memory.
     *
     */
    void update_time();

    /**
     * @brief Advance the day by one.
     */
    void advance_day();
};

/**
 * @brief Holds extra RAM contoller.
 *
 * Writes to RAM when disabled can change banking of ROM.
 */
class Cartridge_MBC3_dis : public Cartridge_RAM {
     bool            _latched;
     uint8_t         _reg;
     uint8_t         _two;

public:
     Cartridge_MBC3  *top;

    /**
     * @brief Create Cartridge RAM space.
     *
     * Create RAM space for a Cartridge, size specifies power
     * of 2 number of bytes to allocation. Also mask off any
     * un-accessible bits of bank select if RAM is too small to
     * have banks. This makes RAM bank bits over size to be
     * effectively ignored.
     *
     * @param size Amount in bytes of RAM to allocate.
     */
    explicit Cartridge_MBC3_dis() {
        _data = NULL;
        _mask = 0xff;
        _size = 0;
        _latched = false;
        _reg = 0;
        _two = 0;
        top = NULL;
    }

    /**
     * @brief Routine to read from memory.
     *
     * Return the value based on the mask to select range of access.
     *
     * @param[out] data Data read from memory.
     * @param[in] addr Address of memory to read.
     */
    virtual void read(uint8_t &data, [[maybe_unused]]uint16_t addr) const override {
          data = 0xff;
    }

    /**
     * @brief Routine to write to memory.
     *
     * Set memory at address to data, based on mask.
     * @param[in] data Data to write to memory.
     * @param[in] addr Address of memory to write.
     */
    virtual void write(uint8_t data, uint16_t addr) override;

    /**
     * @brief Size of RAM in 256 bytes.
     *
     * @return 32 for 8k of RAM.
     */
    virtual size_t size() const override {
       return 32;
    }

};

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
public:
    bool     rtc_flag;       /**< Indicate whether cartridge supports RTC */

    Cartridge_MBC3_bank(uint8_t *data, size_t size) :
                   Cartridge_bank(data, size) {
        rtc_flag = false;
    }

    /**
     * @brief Handle writes to ROM.
     *
     * @param data Data to write to bank register.
     * @param addr Address to write to.
     */
    virtual void write(uint8_t data, uint16_t addr) override;
};

/**
 * @brief Lower bank of MBC3 ROM Cartridge.
 *
 * The lower bank controls enabling of RAM and selection of the lower bank bits.
 *
 * Writing 0xxA to 0x0000-0x1fff enables the RAM. Writing anything else disables
 * RAM. Writing to 0x2000-0x3fff selects lower 7 bits ROM bank cartridges.
 *
 */
class Cartridge_MBC3 : public Cartridge_ROM {
    Cartridge_MBC3_bank  *_rom_bank;
    Cartridge_MBC3_dis    _dis_ram;

    bool                  _extra_banks;
public:
    uint32_t              bank_zero;
    Cartridge_MBC3(Memory *mem, uint8_t *data, size_t size, bool color) :
       Cartridge_ROM(mem, data, size, color) {
        _rom_bank = new Cartridge_MBC3_bank(data, size);
       std::cout << "MBC3 Cartridge";
       _extra_banks = false;
       bank_zero = 0;
       _dis_ram.top = this;
    }

    Cartridge_MBC3(const Cartridge_MBC3 &) = delete;

    Cartridge_MBC3& operator=(const Cartridge_MBC3 &) = delete;

    ~Cartridge_MBC3() {
         delete _rom_bank;
    }

    virtual Cartridge_RAM *set_ram(int type, uint8_t *ram_data,
                                                   size_t ram_size) override;

    /**
     * @brief Map Cartridge into Memory space.
     *
     * Map Cartridge ROM and RAM objects into memory space. Initially the RAM
     * is pointed to Empty until the RAM is enabled. If Boot ROM is enabled
     * (default) the boot ROM is mapped over the lower 256 bytes of ROM.
     */
    virtual void map_cart() override;

    /**
     * @brief Read a byte from ROM.
     *
     * Handle remapping of page 0 for some MBC3 cartridges.
     *
     * @param[out] data Value of memory at address.
     * @param[in] addr Address to access.
     */
    virtual void read(uint8_t &data, uint16_t addr) const override {
       data = _data[bank_zero | addr];
    }

    /**
     * @brief Handle writes to lower bank.
     *
     * Write to lower bank either enable/disables to RAM or
     * Selects the lower 7 bits of the ROM bank.
     */
    virtual void write(uint8_t data, uint16_t addr) override;

    /**
     * @brief Size of non-bank selection area.
     *
     * @return Always return 16K pages.
     */
    virtual size_t size() const override { return 64; }

};
