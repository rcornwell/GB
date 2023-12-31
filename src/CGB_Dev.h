/*
 * GB - Color Game Boy specific devices.
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

#include <stdint.h>
#include "Device.h"
#include "Memory.h"
#include "Ppu.h"
#include "Apu.h"

/**
 * @brief Ram bank control device.
 *
 * Device handles WRAM bank switching on color Game Boy.
 */
class SVBK : public Device {
    RAM      *_ram;         /**< Pointer to Ram memory */
    uint8_t   _bank;        /**< Pointer to current Ram Bank */
    bool      _dis_bank;    /**< Disable bank selection */

public:
    explicit SVBK(RAM *ram) : _ram(ram), _bank(1), _dis_bank(false) {}

    /**
     * @brief RAM bank control read register.
     *
     * Returns last bank selected.
     *
     * @param[out] data Data read from register.
     * @param[in] addr Address of register to read.
     */
     virtual void read_reg(uint8_t &data, [[maybe_unused]]uint16_t addr) const override {
          if (_dis_bank) {
              data = 0xff;
          } else {
              data = 0xf8 | _bank;
          }
     }

    /**
     * @brief RAM bank select register.
     *
     * Pass data to RAM object to set bank.
     *
     * @param[in] data Data write to register.
     * @param[in] addr Address of register to write..
     */
     virtual void write_reg(uint8_t data,
                            [[maybe_unused]]uint16_t addr) override {
          _bank = data & 0x7;
          if (!_dis_bank) {
              _ram->set_bank(_bank);
          }
     }

     /**
      * @brief RAM bank select is one location.
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x70;
     }

     /**
      * @brief RAM bank select is one location.
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 1;
     }

     /**
      * @brief set emulation mode.
      */
     void dis_bank() {
          _dis_bank = true;
     }
};

/**
 * @brief Handle selection of video bank on Color Game Boy.
 *
 * Device handles VRAM bank switching on color Game Boy.
 */
class VBK : public Device {
    Ppu      *_ppu;         /**< Pointer to PPU device */
    uint8_t   _bank;        /**< Video bank cache */

public:
    explicit VBK(Ppu *ppu) : _ppu(ppu), _bank(0) { }

    /**
     * @brief RAM bank control read register.
     *
     * Returns last bank selected.
     *
     * @param[out] data Data read from register.
     * @param[in] addr Address of register to read.
     */
     virtual void read_reg(uint8_t &data, [[maybe_unused]]uint16_t addr) const override {
         if ((_ppu->get_ppu_mode() & 0x4) != 0) {
             data = 0xff;
         } else {
             data = 0xfe | _bank;
         }
     }

    /**
     * @brief RAM bank select register.
     *
     * Pass data to RAM object to set bank.
     *
     * @param[in] data Data write to register.
     * @param[in] addr Address of register to write..
     */
     virtual void write_reg(uint8_t data,
                            [[maybe_unused]]uint16_t addr) override {
         if ((_ppu->get_ppu_mode() & 0x4) == 0) {
             _bank = data & 1;
             _ppu->set_vbank(_bank);
         }
     }

     /**
      * @brief Base address for VBK register
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x4f;
     }

     /**
      * @brief Size of VBK register
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 1;
     }

};

/**
 * @brief Game Boy Undocumented registers.
 *
 * Undocumented register on Color Game Boy
 */
class UNDOC : public Device {
     uint8_t      _reg[8];             /**< Place to hold values in color mode */
     Apu         *_apu;                /**< Pointer to APU to get sample values */
     bool         _enable;             /**< Enable extra registers */

public:

     explicit UNDOC(Apu *apu) : _apu(apu) {
         for (int i = 0; i < 8; i++) {
             _reg[i] = 0;
         }
         _enable = true;
     }

     /**
      * @brief Address of Undocumented registers.
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x72;
     }

     /**
      * @brief Number of Undocumented registers.
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 6;
     }

     virtual void read_reg(uint8_t &data, uint16_t addr) const override {
         switch(addr & 0x7) {
         default:
              data = _reg[addr & 0x7];
              break;
         case 4:
              if (_enable) {
                  data = _reg[addr & 0x7];
              } else {
                  data = 0xff;
              }
              break;
         case 5:
              data = _reg[addr & 0x7] | 0x8f;
              break;
         case 6:
              data = (_apu->s1.sample & 0xf0) | ((_apu->s2.sample >> 4) & 0xf);
              break;
         case 7:
              data = (_apu->s3.sample & 0xf0) | ((_apu->s4.sample >> 4) & 0xf);
              break;
         }
     }

     virtual void write_reg(uint8_t data, uint16_t addr) override {
         _reg[addr & 0x7] = data;
     }

     void set_disable() {
         _enable = false;
     }
};


/**
 * @brief Key registers to control speed and PPU modes.
 *
 * Device handles WRAM bank switching on color Game Boy.
 */
class KEY : public Device {
    Ppu      *_ppu;         /**< Pointer to PPU device */
    Memory   *_mem;         /**< Pointer to Memory device */
    SVBK     *_svbk;        /**< Pointer to Wram Bank */
    VBK      *_vbk;         /**< Pointer to Vram banking */
    ColorPalette *_cpal;    /**< Pointer to color palette */
    UNDOC    *_undoc;       /**< Pointer to undocumented registers */
    uint8_t   _ppu_mode;    /**< Mode for PPU device. */
    bool      _dis_speed;   /**< Disable speed switching */

public:
    bool      sw_speed;     /**< Switch Speed */

    explicit KEY(Ppu *ppu, Memory *mem, SVBK *svbk, VBK *vbk, ColorPalette *cpal, UNDOC *undoc) :
                   _ppu(ppu), _mem(mem), _svbk(svbk), _vbk(vbk), _cpal(cpal), _undoc(undoc) {
        _ppu_mode = 0;
        sw_speed = false;
        _dis_speed = false;
    }

    /**
     * @brief RAM bank control read register.
     *
     * Returns last bank selected.
     *
     * @param[out] data Data read from register.
     * @param[in] addr Address of register to read.
     */
     virtual void read_reg(uint8_t &data, uint16_t addr) const override {
          data = 0xff;
          if (_dis_speed) {
              return;
          }
          if ((addr & 1)) {
              if ((_ppu_mode & 0xc) == 0) {
                  data = (uint8_t)sw_speed;
                  if (_mem->get_speed()) {
                      data |= 0x80;
                  }
              }
          } else {
              if (!_mem->get_disable()) {
                 data = _ppu_mode;
              }
          }
     }

    /**
     * @brief RAM bank select register.
     *
     * Pass data to RAM object to set bank.
     *
     * @param[in] data Data write to register.
     * @param[in] addr Address of register to write..
     */
     virtual void write_reg(uint8_t data, uint16_t addr) override {
          if (_dis_speed) {
              return;
          }
          if ((addr & 1)) {
             sw_speed = (data & 1);
          } else {
              if (!_mem->get_disable()) {
                 _ppu_mode = data;
                 _ppu->set_ppu_mode(data);
                 if ((data & 0x8) != 0) {
                     _dis_speed = true;
                     _mem->dis_hdma = true;
                 }
                 if ((data & 0x4) != 0) {
                     _dis_speed = true;
                     _mem->dis_hdma = true;
                     _svbk->dis_bank();
                     _cpal->set_disable();
                     _undoc->set_disable();
                 }
              }
          }
     }

     /**
      * @brief Base address for Key registers
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x4c;
     }

     /**
      * @brief Size of Key registers
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 2;
     }
};

/**
 * @brief Color Hblank DMA registers.
 *
 */
class HDMA : public Device {
     Memory     *_mem;        /**< Pointer to Memory object */

public:
     explicit HDMA(Memory *mem) : _mem(mem) {}

    /**
     * @brief Read HDMA registers.
     *
     * Returns HDMA values.
     *
     * @param[out] data Data read from register.
     * @param[in] addr Address of register to read.
     */
     virtual void read_reg(uint8_t &data, uint16_t addr) const override {
          if (!_mem->dis_hdma && (addr & 07) == 5) {
              data = _mem->hdma_cnt;
              data |= (_mem->hdma_en) ? 0 : 0x80;
          } else {
              data = 0xff;
          }
     }

    /**
     * @brief Write HDMA registers.
     *
     * Update data in Memory for HDMA transfer.
     *
     * @param[in] data Data write to register.
     * @param[in] addr Address of register to write..
     */
     virtual void write_reg(uint8_t data, uint16_t addr) override {
          if (_mem->dis_hdma) {
              return;
          }
          switch(addr & 07) {
          case 1:
                  _mem->hdma_src = (((uint16_t)data) << 8) | (_mem->hdma_src & 0xf0);
                  break;
          case 2:
                  _mem->hdma_src = (_mem->hdma_src & 0xff00) | ((uint8_t)(data & 0xf0));
                  break;
          case 3:
                  _mem->hdma_dst = (((uint16_t)data) << 8) | (_mem->hdma_dst & 0xf0);
                  break;
          case 4:
                  _mem->hdma_dst = (_mem->hdma_dst & 0x1f00) | ((uint8_t)(data & 0xf0));
                  break;
          case 5:
                  _mem->hdma_cnt = data & 0x7f;
                  if (data & 0x80) {
                      _mem->hdma_en = true;
                  } else {
                      if (_mem->hdma_en) {
                          _mem->hdma_en = false;
                      } else {
                          _mem->hdma_en = true;
                          while(_mem->hdma_en) {
                              _mem->hdma_cycle();
                          }
                      }
                  }
          }
     }

     /**
      * @brief Address of HDMA device.
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x51;
     }

     /**
      * @brief Size of HDMA device.
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 5;
     }
};

/**
 * @brief Game Boy Object Priority mode.
 *
 * Manage the Object Priority mode bit.
 */
class OPRI : public Device {
     Ppu          *_ppu;            /**< Pointer to PPU device */
     uint8_t      _mode;            /**< Priority mode */

public:

     explicit OPRI(Ppu *ppu) : _ppu(ppu), _mode(0) { }

     /**
      * @brief Address of Object priority mode.
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x6c;
     }

     /**
      * @brief Number of registers Object priority mode.
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 1;
     }

     virtual void read_reg(uint8_t &data,
                                [[maybe_unused]]uint16_t addr) const override {
         if ((_ppu->get_ppu_mode() & 0x4) != 0) {
             data = 0xff;
         } else {
             data = _mode | 0xfe;
         }
     }

     virtual void write_reg(uint8_t data,
                                [[maybe_unused]]uint16_t addr) override {
         _mode = data;
         _ppu->set_obj_pri(data);
     }
};


