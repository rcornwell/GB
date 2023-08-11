/*
 * GB - Main Memory objects.
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


#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

/*
 * Classes used in Memory.
 */

class Memory;
class Ppu;
class Apu;
class Timer;
class Serial;

/**
 * @brief Slice of memory.
 *
 * Memory is divided into a series of 256 byte chunks. Each chunk
 * is a slice.
 */
class Slice {
public:
     /**
      * Create a new Slice object.
      */
     Slice() {}

     virtual ~Slice() {}

     /**
      * @brief Routine to read from memory.
      *
      * Each sub-class needs to define these functions.
      * Read data from object.
      * @param[out] data Data read from memory.
      * @param[in] addr Address of memory to read.
      */
     virtual void read(uint8_t &data, uint16_t addr) const = 0;

     /**
      * @brief Routine to write to memory.
      *
      * Each sub-class needs to define these functions.
      * Write data to object.
      * @param[in] data Data to write to memory.
      * @param[in] addr Address of memory to write.
      */
     virtual void write(uint8_t data, uint16_t addr) = 0;

     /**
      * @brief Return size of object in 256 byte values.
      *
      * Each sub-class needs to define these functions.
      * Return number of 256 byte segments in an object.
      * @return Number of slices in object.
      */
     virtual size_t size() const = 0;

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
      virtual int bus() const = 0;
};

/**
 * @brief Empty space in memory.
 *
 * Empty memory always returns 0xff, and ignores all writes.
 * This class can also be used to prevent access to blocks
 * of memory.
 */
class Empty : public Slice {
public:
     /**
      * Create a new Empty object.
      */
     Empty() {}

     /**
      * @brief Routine to read from memory.
      *
      * Always return 0xff to represent an unconnected object.
      * @param[out] data Data read from memory.
      * @param[in] addr Address of memory to read.
      */
     virtual void read(uint8_t &data,
                       [[maybe_unused]]uint16_t addr) const override {
        data = 0xff;
     }

     /**
      * @brief Routine to write to memory.
      *
      * Write data, for Empty memory this does nothing.
      * @param[in] data Data to write to memory.
      * @param[in] addr Address of memory to write.
      */
     virtual void write([[maybe_unused]]uint8_t data,
                 [[maybe_unused]]uint16_t addr) override {
     }

     /**
      * @brief Return size of object in 256 byte values.
      *
      * @return Returns default value of 1.
      */
     virtual size_t size() const override { return 1; }

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
     virtual int bus() const override { return 3; }
};

/**
 * @brief Area represents a generic chunk of memory.
 *
 * An Area represnets a block of memory that can be read and written.
 * It has a pointer to data space, and a mask indicating what address
 * bits are valid. The mask must be a power of 2 - 1.
 */
class Area : public Slice {
protected:
     uint8_t    *_data;     /**< Data to operate on */
     uint16_t    _mask;     /**< Mask of valid address bits */
     size_t      _size;     /**< Size of Area in 256 byte slices. */

public:
     /**
      * @brief Create empty object, with no data.
      */
     Area() : _data(NULL), _mask(0), _size(0) {}

     /**
      * @brief Create an object with 256 bytes of data.
      */

     explicit Area(uint8_t *data) : _data(data), _mask(0xff), _size(1) {}

     /**
      * @brief Create an object with more then 256 bytes of data.
      */
     explicit Area(uint8_t *data, uint16_t mask) : _data(data), _mask(mask) {
         _size = (mask + 1) >> 8;
     }

     /**
      * @brief Create object with given data and size.
      */
     explicit Area(uint8_t *data, size_t size) : _data(data) {
         _mask = size - 1;
         _size = size >> 8;
     }

     /**
      * @brief Routine to read from memory.
      *
      * Return the value based on the mask to select range of access..
      * @param[out] data Data read from memory.
      * @param[in] addr Address of memory to read.
      */
     virtual void read(uint8_t &data, uint16_t addr) const override {
         data = _data[addr & _mask];
     }

     /**
      * @brief Routine to write to memory.
      *
      * Set memory at address to data, based on mask.
      * @param[in] data Data to write to memory.
      * @param[in] addr Address of memory to write.
      */
     virtual void write(uint8_t data, uint16_t addr) override {
         _data[addr & _mask] = data;
     }

     /**
      * @brief Return size of object in 256 byte values.
      *
      * @return Returns default value of 1.
      */
     virtual size_t size() const override { return _size; }

};

/**
 * @brief RAM is a area of memory.
 *
 * RAM is a bank of memory. It is allocated based on the size given.
 */
class RAM : public Area {
     uint16_t    _bank;        /**< Ram bank */

public:
     /**
      * @brief Create area of memory of size bytes.
      *
      * Creates a block of memory. Initialize size to number of
      * 256 byte pages.
      *
      * @param size Size of memory space to create. Must be power of 2.
      */
     explicit RAM(size_t size) {
         _data = new uint8_t[size];
         _mask = 0x0fff;
         _size = 32;
         _bank = 0x1000;
     }

     /**
      * @brief Destructor for RAM.
      *
      * Free memory created.
      */
     ~RAM() { delete[] _data; }

     /**
      * @brief Set bank value.
      *
      * Sets the value of the upper bank. Bank 0 maps to bank 1.
      *
      * @param bank New bank number.
      */
     void set_bank(uint8_t bank) {
          if (bank == 0) {
              bank = 1;
          }
          _bank = ((uint16_t)(bank & 7)) << 12;
     }

     /**
      * @brief Routine to read from memory.
      *
      * Return the value based on the mask to select range of access..
      * @param[out] data Data read from memory.
      * @param[in] addr Address of memory to read.
      */
     virtual void read(uint8_t &data, uint16_t addr) const override {
         if (addr & 0x1000) {
             data = _data[(addr & _mask) | _bank];
         }  else {
             data = _data[addr & _mask];
         }
     }

     /**
      * @brief Routine to write to memory.
      *
      * Set memory at address to data, based on mask.
      * @param[in] data Data to write to memory.
      * @param[in] addr Address of memory to write.
      */
     virtual void write(uint8_t data, uint16_t addr) override {
         if (addr & 0x1000) {
             _data[(addr & _mask) | _bank] = data;
         } else {
             _data[addr & _mask] = data;
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
};


/**
 * @brief Object that represents all Memory for Game Boy.
 *
 * Memory is split into 256 chunks of 256 bytes each. At creation time
 * the memory is set to all empty chunks.
 *
 * Memory also handles DMA operations to OAM space. If a DMA transfer is
 * in progress this will take presentence over access to any Memory except
 * page 0xff.
 *
 * Memory handles cycle timing, every time memory is accessed the timer
 * and ppu cycle function are called to handle background processing of
 * time related functions. The CPU calls internal() if it is not accessing
 * memory.
 */
class Memory {
     Slice     *_mem[256];    /**< Slices of memory */
     Empty      _empty;       /**< Empty space for default memory access */
     bool       _dma_flag;    /**< DMA in operation. */
     uint16_t   _dma_addr;    /**< Current DMA address pointer. */
     int        _dma_count;   /**< Count of current DMA cycle */
     int        _dma_bus;     /**< Current DMA bus */
     Timer     *_timer;       /**< Pointer to Timer object */
     Ppu       *_ppu;         /**< Pointer to PPU object */
     Apu       *_apu;         /**< Pointer to APU object */
     Serial    *_serial;      /**< Pointer to Serial datalink object */
     uint64_t   _cycles;      /**< Number of cycles executed. */
     bool       _color;       /**< Color device */
     bool       _disable_rom; /**< Boot ROM disabled */
     bool       _speed;       /**< Running in double speed mode */
     int        _step;        /**< Cycle step */

public:
     uint16_t   hdma_src;     /**< Source of HDMA transfer */
     uint16_t   hdma_dst;     /**< Destination for HDMA transfer */
     uint8_t    hdma_cnt;     /**< Count of HDMA transfer */
     bool       hdma_en;      /**< Enable HDMA transfer */
     bool       dis_hdma;     /**< HDMA is disabled */

     /**
      * @brief Create default Memory object.
      *
      * @param timer Timer Object used to update timer events.
      * @param ppu  Ppu Object used to update graphics screen.
      * @param apu  Apu Object used to update sound generation.
      * @param serial Serial Object used to clock out data.
      * @param color True if emulating Color Game Boy.
      */
     Memory(Timer *timer, Ppu *ppu, Apu *apu, Serial *serial, bool color) :
            _dma_flag(false), _timer(timer), _ppu(ppu), _apu(apu),
            _serial(serial), _cycles(0), _color(color), _disable_rom(false) {
         /* Map all of memory to empty regions. */
         for (int i = 0; i < 256; i++) {
             _mem[i] = &_empty;
         }
         _dma_flag = false;
         _dma_addr = 0;
         _dma_count = 0;
         _dma_bus = 2;
         _speed = false;
         _step = true;
         hdma_src = 0;
         hdma_dst = 0x8000;
         hdma_cnt = 0x7f;
         hdma_en = false;
         dis_hdma = !color;
     }

     /**
      * @brief Perform cycle update at points in the cycle.
      *
      * Update all devices that need a clock per cycle.
      */
     void cycle();

     /**
      * @brief Preform a HDMA line transfer.
      *
      * Transfer 16 bytes of memory from hdma_src to hdma_dst.
      */
     void hdma_cycle();

     /**
      * @brief Get Speed flag.
      *
      * @return true if Double speed mode.
      */
     bool get_speed() {
          return _speed;
     }

     /**
      * @brief Switch speed.
      */
     void switch_speed();

     /**
      * @brief Get ROM disabled flag.
      *
      * @return ROM disabled flag.
      */
     bool get_disable() {
          return _disable_rom;
     }

     /**
      * @brief Set ROM disable flag.
      *
      * @param data Value of disable register.
      */
     void set_disable(uint8_t data) {
          _disable_rom = (data & 1);
     }

     /**
      * @brief Read memory.
      *
      * Read from memory, use the upper 8 bits of address to select
      * which slices read function to call. This also processes the
      * current DMA cycle and calls the timer and ppu cycle function.
      *
      * @param[out] data Results of read of memory.
      * @param[in] addr Location to read from.
      */
     void read(uint8_t &data, uint16_t addr);

     /**
      * @brief Write memory.
      *
      * Write to memory, use the upper 8 bits of address to select which
      * slices write function to call. This also processes the current
      * DMA cycle and calls the timer and ppu cycle function.
      *
      * @param[in] data Data to write to memory.
      * @param[in] addr Location to write to.
      */
     void write(uint8_t data, uint16_t addr);

     /**
      * @brief Internal cycle.
      *
      * Used to have timer, ppu and DMA processs when CPU is not accessing
      * memory. This also processes the current DMA cycle and calls the
      * timer and ppu cycle function.
      */
     void internal();

     /**
      * @brief Idle cycle.
      *
      * Used during initialization, just bumps cycle count.
      */
     void idle();

     /**
      * @brief Read memory without doing cycle..
      *
      * Read from memory, use the upper 8 bits of address to select
      * which slices read function to call. This is used for tracing and
      * other debug functions.
      *
      * @param[out] data Results of read of memory.
      * @param[in] addr Location to read from.
      */
     void read_nocycle(uint8_t &data, uint16_t addr);

     /**
      * @brief Add a slice at base address of sz slices.
      *
      * Object pointers are inserted into the mem array starting
      * at based and going for sz number of slices.
      *
      * @param[in] slice Pointer to object to insert.
      * @param[in] base Starting address to insert into.
      * @param[in] sz Number of slices to insert.
      */
     void add_slice_sz(Slice *slice, uint16_t base, size_t sz);

     /**
      * @brief Add a slice at base address using objects size.
      *
      * Object pointers are inserted into the mem array starting
      * at based and going for size of object.
      *
      * @param[in] slice Pointer to object to insert.
      * @param[in] base Starting address to insert into.
      */
     void add_slice(Slice *slice, uint16_t base);

     /**
      * @brief Point memory at base to empty.
      *
      * Point memory to empty so the underlying slice can't be
      * accessed, base should point to original mapping point.
      *
      * @param[in] base Starting address to point to empty
      */
     void free_slice(uint16_t base);

     /**
      * @brief Read DMA register.
      *
      * @param[out] data Data that is read to the DMA register.
      */
     void read_dma(uint8_t& data);

     /**
      * @brief Start a DMA operation by writing to DMA controller.
      *
      * Store the address to transfer into dma_addr and enable the
      * data transfer.
      *
      * @param data Data that is written to the DMA register.
      */
     void write_dma(uint8_t data);

     /**
      * @brief Return current number of cycles executed.
      *
      * @return number of cycles.
      */
     uint64_t get_cycles() const {
         return _cycles;
     }

     /**
      * @brief Adjust for minor cycles.
      *
      * Reset number of cyles based on how many were requested.
      *
      * @param max_cycles Maximum number of CPU cycles we should have ran.
      */
     void reset_cycles(int max_cycles) {
         _cycles = _cycles - max_cycles;
     }
};

