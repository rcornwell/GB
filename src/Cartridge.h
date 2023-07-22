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

#pragma  once

#include <cstdint>
#include <iostream>
#include "Memory.h"
#include "ROM.h"

/**
 * @brief Holds cartridge ram if any.
 *
 * @class Cartridge_RAM Cartridge.h "Cartridge.h"
 *
 * Cartridge ram is mapped to addresses 0xa000-0xbfff. Some can have
 * multiple banks of RAM, this is controlled by mapper hardware. Similar
 * to RAM object, but supports paging. Cartridges can have anywhere from
 * 0 bytes of memory to 32Kbytes of memory.
 */
class Cartridge_RAM : public Area {
protected:
    uint32_t    _bank;           /**< Bank number currently selected */
    uint32_t    _bank_mask;      /**< Mask for setting bank bits */
    bool        _need_delete;    /**< Flag if we created data */
public:

    /**
     * @brief Create Cartridge RAM space.
     *
     * Create RAM space for a Cartridge, size specifies power
     * of 2 number of bytes to allocation. Also mask off any
     * unaccessable bits of bank select if RAM is too small to
     * have banks. This makes RAM bank bits over size to be
     * effectively ignored.
     *
     * @param size Amount in bytes of RAM to allocate.
     */
    explicit Cartridge_RAM(size_t size) : _bank(0), _need_delete(true) {
        _data = new uint8_t [size];
        _mask = 0x1fff;
        _size = size;
        _bank_mask = (size - 1) & 0xffe000;
    }

    /**
     * @brief Create a Cartridge RAM from file.
     *
     * Create RAM space for a Cartidge, size is number of bytes 
     * in file, and data points to existing data.
     *
     * @param data Pointer to data to access.
     * @param size Size of data in bytes.
     */
    explicit Cartridge_RAM(uint8_t *data, size_t size) : 
                 _bank(0), _need_delete(false) {
        _data = data;
        _mask = 0x1fff;
        _size = size;
        _bank_mask = (size - 1) & 0xffe000;
    }

    /**
     * @brief Destructor for Cartridge RAM.
     *
     * Free memory created.
     */
    ~Cartridge_RAM() { 
         if (_need_delete) {
             delete[] _data;
         }
     }


    /**
     * @brief Routine to read from memory.
     *
     * Return the value based on the mask to select range of access..
     * @param[out] data Data read from memory.
     * @param[in] addr Address of memory to read.
     */
    virtual void read(uint8_t &data, uint16_t addr) const override {
       data = _data[_bank + (addr & _mask)];
    }

    /**
     * @brief Routine to write to memory.
     *
     * Set memory at address to data, based on mask.
     * @param[in] data Data to write to memory.
     * @param[in] addr Address of memory to write.
     */
    virtual void write(uint8_t data, uint16_t addr) override {
       _data[_bank + (addr & _mask)] = data;
    }

    /**
     * @brief Returns size of object in 256 byte values.
     *
     * If memory is larger then 8K return 32 pages max. Otherwise
     * return the number of 256 pages.
     * @return number of 256 byte pages for object.
     */
    virtual size_t size() const override {
        size_t sz = _size >> 8;
        if (sz > 32) {
             return 32;
        } else {
             return sz;
        }
    }

     /**
      * @brief Return bus number of slice.
      *
      * Used to manage DMA transfers the bus number needs to
      * be compared with the transfer page to determine how 
      * access will occur.
      *
      * Bus number 0 has ROM, and external memory.
      * Bus number 1 has video ram.
      * Bus number 2 has OAM memory.
      * Bus number 3 is internal to the CPU.
      *
      * @return Bus number.
      */
     virtual int bus() const override { return 0; }

    /**
     * @brief Return size of RAM in bytes.
     *
     * @return number of bytes in RAM space.
     */
    virtual size_t size_bytes() {
        return _size;
    }

    /**
     * @brief Set bank of memory to access.
     *
     * Sets the bank to the given value. The bank is shifted
     * to be a multiple of 8192 bytes.
     * @param bank address of base of existing ram.
     */
    void set_bank(uint32_t bank) {
        _bank = bank & _bank_mask;
    }

    /**
     * @brief Return RAM data pointer.
     *
     * @return pointer to ram data.
     */
    uint8_t *ram_data() const {
           return _data;
    }
};

/**
 * @brief Upper 16K of ROM for banked system.
 *
 * @class Cartridge_bank Cartridge.h "Cartridge.h"
 *
 * Each bank controller should overide these methods to support
 * the bank switching system in use.
 */
class Cartridge_bank : public Slice {
protected:
    uint8_t        *_data;         /**< Pointer to base of ROM */
    size_t          _size;         /**< Size of ROM in bytes */
    uint32_t        _mask;         /**< Mask of valid ROM address bits */
    uint32_t        _bank;         /**< Current bank number */
    uint32_t        _ram_bank;     /**< Current RAM bank */
    Cartridge_RAM  *_ram;          /**< Pointer to RAM object */
public:
    /**
     * @brief Create a bank object.
     *
     * Create a bank object, that will access the upper 16K of 
     * ROM space with bank selection.
     * Generally Memory mappers will override this class.
     *
     * @param data  Pointer to ROM image.
     * @param size  Size of ROM image in bytes.
     */
    explicit Cartridge_bank(uint8_t *data, size_t size) :
           _data(data), _size(size), _bank(0x4000), _ram_bank(0),_ram(NULL) {
        _mask = size - 1;
    }

    /**
     * @brief Sets pointer to RAM object.
     *
     * Bank controller needs to know this since RAM bank selection 
     * is typically done in the address range of the BANK.
     *
     * @param ram  Pointer to Cartridge RAM object.
     */
    virtual void set_ram(Cartridge_RAM *ram) {
         _ram = ram;
    }

    /**
     * @brief Sets the bank address.
     *
     * Sets the base address to access the ROM at.
     *
     * @param bank Address of base of ROM bank.
     */
    virtual void set_bank(uint32_t bank) {
         _bank = bank & _mask;
    }

    /**
     * @brief Returns the current bank address.
     *
     * Returns current bank address.
     *
     * @return bank address.
     */
    virtual uint32_t get_bank() const {
        return _bank;
    }

    /**
     * @brief Returns the operating mode of controller.
     *
     * Returns current operating mode of the controller.
     *
     * @return false since default controller does not support mode.
     */
    virtual bool get_mode() const {
        return false;
    }

    /**
     * @brief Read a byte from ROM.
     *
     * Mask Addess give to 16K and then add in bank number to
     * access ROM.
     *
     * @param[out] data Value of memory at address.
     * @param[in] addr Addess to access.
     */
    virtual void read(uint8_t &data, uint16_t addr) const override {
       data = _data[_bank+(addr & 0x3fff)];
    }

    /**
     * @brief Write a byte to ROM.
     *
     * ROM's can't be written so default is to do nothing.
     *
     * @param[in] data Value to write to memory at address.
     * @param[in] addr Addess to access.
     */
    virtual void write([[maybe_unused]]uint8_t data, 
                       [[maybe_unused]]uint16_t addr) override {
    }

    /**
     * @brief Return size of bank section.
     *
     * Returns the size of the banked section of ROM in terms
     * of 256 byte chunks. Bank area of ROM is always 16K.
     *
     * @return 64 256 byte chunks.
     */
    virtual size_t size() const override { return 64; }

     /**
      * @brief Return bus number of slice.
      *
      * Used to manage DMA transfers the bus number needs to
      * be compared with the transfer page to determine how 
      * access will occur.
      *
      * Bus number 0 has ROM, and external memory.
      * Bus number 1 has video ram.
      * Bus number 2 has OAM memory.
      * Bus number 3 is internal to the CPU.
      *
      * @return Bus number.
      */
     virtual int bus() const override { return 0; }
};

/**
 * @brief Represent the ROM Cartridge to the system.
 *
 * @class Cartridge_ROM Cartridge.h "Cartridge.h"
 *
 * Default is that the complete cartridge is accessable at one time.
 * Hence no need to for bank controller. Some memory Cartridges will
 * Override this to provide selection for these features.
 */
class Cartridge_ROM : public Slice {
protected:
    Empty            _empty;        /**< Empty object for RAM area */
    ROM              _rom;          /**< Bootstrap ROM object */
    Memory          *_mem;          /**< Pointer to Memory object */
    uint8_t         *_data;         /**< Data to return */
    size_t           _size;         /**< Size of ROM in bytes */
    bool             _rom_enable;   /**< Boot ROM enabled */
    Cartridge_RAM   *_ram;          /**< Pointer to RAM object if any */

public:
    /**
     * @brief Create a ROM object.
     *
     * Create a ROM object that will serve out data to CPU.
     *
     * @param mem   Pointer to memory object so it can map pages correctly.
     * @param data  Pointer to ROM image.
     * @param size  Size of ROM image in bytes.
     */
    Cartridge_ROM(Memory *mem, uint8_t *data, size_t size) :
        _mem(mem), _data(data), _size(size), _rom_enable(true),
        _ram(NULL) {
    }

    /**
     * @brief Sets pointer to RAM object.
     *
     * Bank controller needs to know this since RAM bank selection 
     * is typically done in the address range of the BANK.
     *
     * @param ram  Pointer to Cartridge RAM object.
     */
    virtual void set_ram(Cartridge_RAM *ram) {
         _ram = ram;
    }

    virtual void map_cart() {
        /* Map ourselves in place */
        _mem->add_slice(this, 0);
        if (_ram) {
            _mem->add_slice(_ram, 0xa000);
        }
        if (_rom_enable) {
            _mem->add_slice(&_rom, 0);
        }
    }

    /**
     * @brief Read a byte from ROM.
     *
     * If no mapper ROM can extend up to 32K, otherwise only access
     * page 0 of ROM.
     *
     * @param[out] data Value of memory at address.
     * @param[in] addr Addess to access.
     */
    virtual void read(uint8_t &data, uint16_t addr) const override {
       data = _data[addr];
    }

    /**
     * @brief Write a byte to ROM.
     *
     * ROM's can't be written so default is to do nothing.
     *
     * @param[in] data Value to write to memory at address.
     * @param[in] addr Addess to access.
     */
    virtual void write([[maybe_unused]]uint8_t data,
                       [[maybe_unused]]uint16_t addr) override {
    }

    /**
     * @brief Return size of bank section.
     *
     * Returns the size of the banked section of ROM in terms
     * of 256 byte chunks. If no banking, then ROM can be up
     * 128 pages (32K). If over 128 pages, it must be banked hence
     * lower ROM is limited to 16K bytes of ROM.
     *
     * @return size of ROM in 256 byte chunks.
     */
    virtual size_t size() const override { 
          size_t sz = _size >> 8;
          return (sz > 128) ? 64 : sz;
    }

     /**
      * @brief Return bus number of slice.
      *
      * Used to manage DMA transfers the bus number needs to
      * be compared with the transfer page to determine how 
      * access will occur.
      *
      * Bus number 0 has ROM, and external memory.
      * Bus number 1 has video ram.
      * Bus number 2 has OAM memory.
      * Bus number 3 is internal to the CPU.
      *
      * @return Bus number.
      */
     virtual int bus() const override { return 0; }

    /**
     * @brief Hooked to I/O page to control Boot ROM access.
     *
     * When CPU access 0xff50, this routine is called to enable
     * or disable to Boot ROM. If enabled the Boot ROM is mapped
     * over the first page of the ROM. When disabled the full ROM
     * is enabled.
     *
     * @param data Value written to 0xff50.
     */
    virtual void disable_rom(uint8_t data) {
         _rom_enable = !(data & 1);
         if (_rom_enable) {
             _mem->add_slice(&_rom, 0);
         } else {
             _mem->add_slice_sz(this, 0, _rom.size());
         }
    }
};

enum Cart_type {
    ROM, MBC1, MBC2, MBC3, MBC5, MMM01
};                                /**< Types of Cartridges. */

#define CRAM 0x100 
#define BAT 0x200
#define TIM 0x400

/**
 * @brief Top level of ROM Cartridge.
 *
 * @class Cartridge Cartridge.h "Cartridge.h"
 *
 * Created before the CPU is created, this object holds the current
 * ROM and RAM of the cartridge.
 *
 */
class Cartridge {
    Cartridge_ROM  *_rom;      /**< Pointer to base object of ROM */
    Cartridge_RAM  *_ram;      /**< Pointer to RAM for Cartridge */
    Memory         *_mem;      /**< Pointer to memory object when created. */
    uint8_t        *_data;     /**< Actual ROM data. */
    size_t          _size;     /**< Size of ROM in bytes */
    uint8_t        *_ram_data; /**< Loaded copy of RAM data */
    size_t          _ram_size; /**< Size of RAM data */

public:
    Cartridge() : _rom(NULL), _ram(NULL), _mem(NULL), _data(NULL),
                 _size(0), _ram_data(NULL), _ram_size(0) {}

    ~Cartridge() {
        delete _rom;
        delete _ram;
    }

    void set_ram();

    void load_ram(uint8_t *data, size_t size);

    bool ram_battery();

    uint8_t *ram_data() const {
         if (_ram) {
             return _ram->ram_data();
         } else {
             return NULL;
         }
    }

    size_t ram_size() const {
         return _ram_size;
    }

    void set_rom(uint8_t *data, size_t size);

    void set_mem(Memory *mem);

    void disable_rom(uint8_t data) {
         if (_rom != NULL) {
             _rom->disable_rom(data);
         }
    }
};
