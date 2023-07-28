/*
 * GB - Picture Processoring Unit.
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

#include <cstdint>
#include <iostream>

#include "Memory.h"
#include "Device.h"

/**
 * @brief Layout of Game Boy Graphics processor memory space.
 *
 * Tile Data Table:   0x8000 - 0x8fff  (0 - 256)
 *                    0x8800 - 0x97ff  (-128 - 127)
 * Tile Background    0x9800 - 0x9bff  Area 0
 *                    0x9c00 - 0x9fff  Area 1
 *
 * OAM                0xfe00 - 0xfe9f
 *
 * LCDC:   0xff40
 *
 *                    Bit 0   BG and Window Enable 
 *                    Bit 1   OBJ enable
 *                    Bit 2   OBJ size
 *                    Bit 3   BG tile map      0x9800-0x9bff, 0x9c00-0x9fff
 *                    Bit 4   BG tile data     0x8800-0x97ff, 0x8000-0x8fff
 *                    Bit 5   Window enable
 *                    Bit 6   Window tile area 0x9800-0x8bff, 0x9c00-0x9fff
 *                    Bit 7   LCD and PPU enable
 *
 * STAT:   0xff41     Bit 0    Mode flag 1 (RO)
 *                    Bit 1    Mode flag 2 (RO)
 *                    Bit 2    LYC=LY flag (RO)
 *                    Bit 3    Mode 0 HBlank interrupt. (RW)
 *                    Bit 4    Mode 1 VBlank interrupt. (RW)
 *                    Bit 5    Mode 2 OAM interrupt.    (RW)
 *                    Bit 6    LYC=LY interrupt.        (RW)
 *                    Bit 7    (nothing)
 *
 * SCY:    0xff42     Starting Y to display
 * SCX:    0xff43     Starting X to display
 *
 * LY:     0xff44     Y coordinate  (RO)
 *
 * LYCL    0xff45     LY Compare register.
 *
 * DMA:    0xff46     Page to DMA from.
 *
 * BGP     0xff47     Bit 1-0    Color index 0
 *                    Bit 3-2    Color index 1
 *                    Bit 5-4    Color index 1
 *                    Bit 7-6    Color index 1
 *
 * OBP0    0xff48     Bit 1-0    Color index 0
 *                    Bit 3-2    Color index 1
 *                    Bit 5-4    Color index 1
 *                    Bit 7-6    Color index 1
 *
 * OBP1    0xff49     Bit 1-0    Color index 0
 *                    Bit 3-2    Color index 1
 *                    Bit 5-4    Color index 1
 *                    Bit 7-6    Color index 1
 *
 * WY:     0xff4A     Window Starting Y to display
 * WX:     0xff4B     Window Starting X to display
 *
 *                           | Row  | Col | Dot
 *                           76 543  7 6543 210
 *                    1001 1000 000  0 0000 
 *                    1001 1011 111  1 1111
 */

/**
 * @class Tile_data
 * @brief Tile Data area of Memory.
 *
 * Tiles are stored in even/odd byte pairs. The even byte gives the 
 * lower bit of each pixel, the odd byte gives the upper bit of each
 * pixed. 8 pixels are packed into 2 bytes.
 *
 * The Tile Memory converts bytes into 2 bit tile pieces whenever
 * any of the bytes are updated. This simplifes the processing of 
 * pixels when it is being displayed. Each row of the tiles is stored
 * as an 8 byte array.
 *
 */
class Tile_data : public Slice {
      uint8_t     _data[6144];        /**< Copy of RAM */
public:
      uint8_t     _tile[3072][8];     /**< Expanded pixel data */

      Tile_data() {
          for (int i = 0; i < 6144; i++) {
               _data[i] = 0;
          }
          for (int i = 0; i < 3072; i++) {
              for (int j = 0; j < 8; j++) {
                 _tile[i][j] = 0;
              }
          }
      }

      /**
       * @brief Read tile Data.
       *
       * Read backup copy of data.
       * @param[out] data Data read from memory.
       * @param[in] addr Address of location to read.
       */
      virtual void read(uint8_t &data, uint16_t addr) const override {
           data = _data[addr & 0x1fff];
      }

      /**
       * @brief Write tile data.
       *
       * Write data to tile memory. Update backup copy of
       * data first, then read in the even and odd byte and 
       * reconstruct the pixel data for later use.
       * @param[out] data Data write to memory.
       * @param[in] addr Address of location to read.
       */
      virtual void write(uint8_t data, uint16_t addr) override {
           uint8_t    high;      /* High order byte */
           uint8_t    low;       /* Low order byte */
           uint8_t    mask;      /* Current working mask */
           int        i;         /* Current pixel to update */

           /* Update backup data */
           addr &= 0x1fff;
           _data[addr] = data;

           /* Fetch lower and upper pixel data */
           low = _data[addr & 0x1ffe];
           high = _data[addr | 1];
           addr >>= 1;
           i = 0;
           /* Copy over the high and low bit to form new pixel. */
           for (i = 0, mask = 0x80; mask != 0; i++, mask >>= 1) {
               _tile[addr][i] = (!!(low & mask)) | (!!(high & mask)) << 1;
           }
      }

      /**
       * @brief Return size of tile object.
       *
       * Tile object is fixed at 6144 bytes.
       */
      virtual size_t size() const override {
           return 24;
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
     virtual int bus() const override { return 1; }
};
 
/**
 * @class Tile_map
 * @brief Tile Map.
 *
 * The tile map is just a small section of memory. It is
 * at offset of 0x9800 so this needs to be subtracted before
 * access.
 */
class Tile_map : public Slice {
public:
      uint8_t     _data[2048];     /**< Tile map data */

      Tile_map() {
          for (int i = 0; i < 2048; i++) {
               _data[i] = 0;
          }
      }

      /**
       * @brief Read tile map Data.
       *
       * Read data in tile map.
       * @param[out] data Data read from memory.
       * @param[in] addr Address of location to read.
       */
      virtual void read(uint8_t &data, uint16_t addr) const override {
           data = _data[addr - 0x9800];
      }

      /**
       * @brief Write tile map Data.
       *
       * Write data to tile map.
       * @param[in] data Data read from memory.
       * @param[in] addr Address of location to read.
       */
      virtual void write(uint8_t data, uint16_t addr) override {
           _data[addr - 0x9800] = data;
      }

      /**
       * @brief Tile map is 2k in size.
       */
      virtual size_t size() const override {
           return 8;
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
     virtual int bus() const override { return 1; }
};

/**
 * @brief Used to hold objects to be display on a given line.
 *
 */
struct OBJ {
     uint8_t     X;        /**< X location */
     uint8_t     Y;        /**< Y location */
     uint8_t     flags;    /**< Cached flags */
     uint8_t     tile;     /**< Cached tile */
};

#define LCDC_ENABLE  0x80  /* Enable the LCD controller */
#define WIND_AREA    0x40  /* 0=9800, 1=9c00 */
#define WIND_ENABLE  0x20  /* Enable window */
#define TILE_AREA    0x10  /* 0=8800, 1=8000 */
#define BG_AREA      0x08  /* 0=9800, 1=9c00 */
#define OBJ_SIZE     0x04  /* 0=8x8, 1=8x16 */
#define OBJ_ENABLE   0x02  /* Enable objects */
#define BG_PRIO      0x01  /* 0=white, 1 enabled */

#define OAM_BG_PRI   0x80  /* 1 BG and windows 1-3 over OBJ */
#define OAM_Y_FLIP   0x40  /* Flip vertically */
#define OAM_X_FLIP   0x20  /* Flip horizontally */
#define OAM_PAL      0x10  /* Palette number */

#define STAT_LYC_F   0x04  /* Line counter match flag */
#define MODE_0_IRQ   0x08  /* Post IRQ on entry to MODE 0 HBlank */
#define MODE_1_IRQ   0x10  /* Post IRQ on entry to MODE 1 VBlank */
#define MODE_2_IRQ   0x20  /* Post IRQ on entry to MODE 2 OAM scan */
#define STAT_LYC_IRQ 0x40  /* Post IRQ on LYC Match */

/**
 * @class OAM
 * @brief OAM memory holds Object data values.
 *
 * OAM holds object to display. Each OBJ holds the sprites X,Y
 * coordinates, some flags and the tile to display.
 * At the beginning of each display cycle, OAM scan is called to 
 * select up to 10 objects to display on the given line.
 */
class OAM : public Slice {
      uint8_t    _data[256];      /**< OAM data */
public:
      OBJ        _objs[10];       /**< Sorted object list */

      OAM() {
          for (int i = 0; i < 256; i++) {
               _data[i] = 0;
          }
          /* Initialize objects to invalid */
          for (int obj = 0; obj < 10; obj++) {
              _objs[obj].X = 0xff;
              _objs[obj].Y = 0xff;
              _objs[obj].flags = 0;
              _objs[obj].tile = 0;
          }
      }

      /* Scan OAM, documented with code. */
      void scan_oam(int row, uint8_t lcdc);
 
      /**
       * @brief Read OAM object data.
       *
       * OAM only holds 160 objects, so anything above this returns
       * 0xff. So we check if over range and return 0xff.
       *
       * @param[out] data Data read from memory.
       * @param[in] addr Address of location to read.
       */
      virtual void read(uint8_t &data, uint16_t addr) const override {
           addr &= 0xff;
           if (addr < 160)
              data = _data[addr];
           else
              data = 0xff;
      }

      /**
       * @brief Write OAM object data.
       *
       * The OAM memory array is defined to be 256 bytes long,
       * writes to locations about 160, can occur safely and
       * will not be visible.
       *
       * @param[in] data Data to write from memory.
       * @param[in] addr Address of location to write.
       */
      virtual void write(uint8_t data, uint16_t addr) override {
           _data[addr & 0xff] = data;
      }

      /**
       * @brief OAM data is one page long.
       */
      virtual size_t size() const override {
           return 1;
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
     virtual int bus() const override { return 2; }
};

enum Fetcher_State { GETA, GETB, DATALA, DATALB, 
                     DATAHA, DATAHB, RDY };
enum Fetcher_type  { NONE, BG, WIN, OBJ };
/**
 * @class Ppu
 * @brief Main Picture Processing Unit Object.
 *
 * This object is responsible for generating the screen image every
 * display cycle.
 */
class Ppu : public Device {
     Tile_data   _data;            /**< Tile Data object */
     Tile_map    _map;             /**< Tile Map object */
     OAM         _oam;             /**< OAM object. */
     Memory     *_mem;             /**< Pointer to Memory object */
     
     uint8_t     LCDC;             /**< Display control */
     uint8_t     STAT;             /**< Display status and IRQ flags */
     uint8_t     SCY;              /**< Location of main display */
     uint8_t     SCX;              /**< Location of main display */
     uint8_t     LY;               /**< Current line being processed */
     uint8_t     LYC;              /**< Line compare register */
     uint16_t    LX;               /**< Current pixel being processed */
     uint8_t     BGP;              /**< Background palette */
     uint8_t     OBP0;             /**< Object 0 palette */
     uint8_t     OBP1;             /**< Object 1 palette */
     uint8_t     WX;               /**< Window display location */
     uint8_t     WY;               /**< Window display location */

     int         _mode;            /**< Current mode */
     bool        _next_line;       /**< Start of new line */
     bool        _enable;
     bool        _start;           /**< Starting new mode */
     bool        _wind_en;         /**< Window ok during line */
     bool        _wind_flg;        /**< Grab from window over background */
     int         _obj_num;         /**< Current displaying object */

     uint8_t     _pix_fifo[16];    /**< Pixel fifo */
     uint8_t     _obj_fifo[8];     /**< Object fifo */
     int         _pix_count;       /**< Number of pixels left in fifo */
     int         _dot_clock;       /**< Current dot position on row */
     Fetcher_State _f_state;       /**< Pixel Fetcher state */
     Fetcher_type  _f_type;        /**< Current data in fetcher */

     int         _wrow;            /**< Current window row */
     int         _wline;           /**< Current line of window */
     int         _wtile;           /**< Current window tile */
     int         _brow;            /**< Current background row */
     int         _btile;           /**< Current background tile */

public:

     /**
      * @brief Initialize a Ppu object.
      *
      * Set default Palettes and mode to Vblank.
      */
     Ppu() {
         int   i;
         _mem = NULL;
         LX = LY = 0;
         LYC = 0;
         SCX = SCY = WX = WY = 0;
         STAT = 0;
         LCDC = 0;
         BGP = 0xfc;
         OBP0 = 0xff;
         OBP1 = 0xff;
         _start = true;
         _enable = false;
         _next_line = false;
         _mode = 1;
         _wind_en = false;
         _wind_flg = false;
         _obj_num = 0;
         _wline = _wtile = _wrow = _brow = _btile = 0;
         _dot_clock = 0;
         _f_state = RDY;
         _f_type = NONE;
         /* Clear fifos */
         for (i = 0; i < 16; i++) {
             _pix_fifo[i] = 0;
         }
         for (i = 0; i < 8; i++) {
             _obj_fifo[i] = 0;
         }

         _pix_count = 0;
     }

     /**
      * @brief Address of APU unit.
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x40;
     }

     /**
      * @brief Number of registers APU unit has.
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 12;
     }

     /**
      * @brief Connect to memory object.
      *
      * Set memory object pointer. Also map display items into
      * memory so they can be modified.
      *
      * @param mem Pointer to Memory object.
      */
     void set_mem(Memory *mem) {
          _mem = mem;
          _mem->add_slice(&_data, 0x8000);
          _mem->add_slice(&_map, 0x9800);
          _mem->add_slice(&_oam, 0xfe00);
     }

     void check_lyc();

     void enter_mode0();

     void enter_mode1();

     void enter_mode2();

     virtual void read_reg(uint8_t &data, uint16_t addr) const override;

     virtual void write_reg(uint8_t data, uint16_t addr) override;

     virtual void cycle() override;

     void fill_pix(int tile, int st_pix, int row);

     void shift_fifo();

     void display_start();

     void display_pixel();
     
};
