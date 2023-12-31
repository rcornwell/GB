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


#include <cstdint>
#include <iostream>
#include "Ppu.h"
#include "Cpu.h"
#include "System.h"

#define TY  0
#define TX  1
#define TT  2
#define TF  3

/**
 * @brief Scan OAM memory and choose 10 object to display.
 *
 * OAM scan starts with marking the 10 objects as invalid and off the
 * screen. If OBJ display is not enabled, exit now. Next read each
 * object's Y position and decide if it is to be displayed now or not.
 * If the last object in our array is being displayed we can stop and
 * start display process. If not we scan the array of objects looking for
 * the first one that is after the current one in the line. Once we find
 * one we slide all objects down one in the array.
 *
 * @param row      Current row to process.
 * @param lcdc     Current value of PPU control register.
 * @param obj_pri  Priority give to object (CGB)
 */
void OAM::scan_oam(int row, uint8_t lcdc, uint8_t obj_pri)
{
    int    obj;                               /* Current object been check */
    int    ins;                               /* Insertion point */
    int    high = (lcdc & OBJ_SIZE) ? 16: 8;  /* Hieght of objects. */

    /* Initialize objects to invalid */
    for (obj = 0; obj < 10; obj++) {
        objs[obj].X = 0xff;
    }

    /* If Objects not enabled, just return */
    if ((lcdc & OBJ_ENABLE) == 0) {
        return;
    }

    /* Scan object memory to find where to put things */
    for (obj = 0; obj < 160; obj += 4) {
         int Y = (int)_data[obj+TY];
         uint8_t  X = _data[obj+TX];

         /* Check if we have 10 objects */
         if (objs[9].X != 0xff) {
             break;
         }
         /* Check if object in display area. */
         if (row < Y || row >= (Y + high)) {
            continue;
         }

         /* Scan objects looking for first one with X larger */
         for (ins = 0; ins < 10; ins++) {
             if (objs[ins].X == 0xff) {
                 break;
             }
             if (X <= objs[ins].X) {
                 break;
             }
         }

         /* If past end of array, exit */
         if (ins == 10) {
             break;
         }

         /* If X the same keep original */
         if (objs[ins].X == X) {
            while(ins < 10 && objs[ins].X == X) {
                ins++;
            }
         }

         /* If no more space break out */
         if (ins == 10) {
             break;
         }

         /* If X larger, shift later objects down */
         if (X != objs[ins].X && objs[ins].X != 0xff) {
             for (int i = 9; i >= ins && i > 0; i--) {
                 objs[i] = objs[i-1];
             }
         }
         objs[ins].X = X;
         objs[ins].Y = _data[obj+TY];
         objs[ins].flags = _data[obj+TF];
         objs[ins].tile = _data[obj+TT];
         objs[ins].num = obj;
     }
}

void OAM::print_oam()
{
    for (int i = 0; i < 160; i +=4) {
        printf("Obj: %d X=%d Y=%d NUM=%02x A=%02x\n", i/4, _data[i+TX],
            _data[i+TY] - 16, _data[i+TT], _data[i+TF]);
    }
}

void OAM::print_sort_oam()
{
    for (int i = 0; i < 10; i ++) {
        printf("Obj: %d X=%d Y=%d NUM=%02x A=%02x\n", i, objs[i].X,
             objs[i].Y - 16, objs[i].tile, objs[i].flags );
    }
}

void Tile_data::print_map(int num)
{
    int i, l, k, j;
    printf("Tile Data %d:\n", num);
    for (i = 0; i < 3072; i+=64) {
        printf("%03x: ", i >> 3);
        for (l = 0; l < 8; l++) {
            for (k = 0; k < 64; k+=8) {
                for (j = 0; j < 8; j ++) {
                    printf("%d", tile[i+l+k][j]);
                }
                printf("|");
            }
            if (l < 7) {
                printf("\n     ");
            }
        }
        printf("\n");
    }
    printf("\n");
}

/**
 * @brief Read Palette control registers.
 *
 * Read control registers.
 *
 * @param[out] data Data read from register.
 * @param[in]  addr Register to read.
 */
void ColorPalette::read_reg(uint8_t &data, uint16_t addr) const {
    /* Ignore reads here when in compatible mode */
    if (!_enable) {
        data = 0xff;
        return;
    }
    switch (addr & 0x3) {
    case 0:    /* Background color control register */
            data = _bg_ctrl | 0x40;
            break;

    case 1:    /* Read background palette at value */
            data = _palette[_bg_ctrl & 0x3f];
            break;

    case 2:    /* Object color control register */
            data = _obj_ctrl | 0x40;
            break;

    case 3:    /* Read Object palette at value */
            data = _palette[(_obj_ctrl & 0x3f) | 0x40];
            break;
    }
}

/**
 * @brief Write Palette control registers.
 *
 * Write control registers.
 *
 * @param[out] data Data write to register.
 * @param[in]  addr Register to write.
 */
void ColorPalette::write_reg(uint8_t data, uint16_t addr) {
    int     num;
    switch (addr & 0x3) {
    case 0:    /* Background color control register */
            _bg_ctrl = data;
            break;

    case 1:    /* Write background palette at value */
            num = _bg_ctrl & 0x3f;
            _palette[num] = data;
            set_palette_col((num >> 1), _palette[num & 0x7e], _palette[num | 1]);
            if (_bg_ctrl & 0x80) {
                _bg_ctrl = (_bg_ctrl & 0xc0) | ((num + 1) & 0x3f);
            }
            break;

    case 2:    /* Object color control register */
            _obj_ctrl = data;
            break;

    case 3:    /* Write Object palette at value */
            num = (_obj_ctrl & 0x3f) | 0x40;
            _palette[num] = data;
            set_palette_col((num >> 1), _palette[num & 0x7e], _palette[num | 1]);
            if (_obj_ctrl & 0x80) {
                _obj_ctrl = (_obj_ctrl & 0xc0) | ((num + 1) & 0x3f);
            }
            break;
    }
}

/**
 *
 * @brief Handle interrupt posting.
 *
 * @param value Bit to set in IF register.
 */

void Ppu::post_irq(uint8_t value)
{
     /* Check if vblank interrupt */
     if (value == VBLANK_IRQ) {
         if (_irq_flg) {
            *_irq_flg |= value;
         }
         return;
     }

     /* Save current value of _irq_stat */
     uint8_t  _prev_irq = _irq_stat;

     _irq_stat = 0;
     /* Update mode bits */
     switch(_mode) {
     case 0:      _irq_stat = MODE_0_IRQ;
                  break;
     case 1:      _irq_stat = MODE_1_IRQ;
                  break;
     case 2:      _irq_stat = MODE_2_IRQ;
                  break;
     case 3:      break;
     }

     /* If MODE 2 IRQ and LY == 144, trigger interrupt */
     if (LY == 144 && (STAT & MODE_2_IRQ) != 0) {
        _irq_stat |= MODE_2_IRQ;
     }

     /* Update LYC status */
     if ((STAT & STAT_LYC_F) != 0) {
         _irq_stat |= STAT_LYC_IRQ;
     }

     /* Clear unmasked bits */
     _irq_stat &= STAT & (STAT_LYC_IRQ|MODE_2_IRQ|MODE_1_IRQ|MODE_0_IRQ);

     /* If interrupts are now pending post them */
     if (_irq_stat != 0 && _prev_irq == 0) {
         if (_irq_flg) {
            *_irq_flg |= value;
         }
     }
}


/**
 * @brief Post LYC match interrupt if match.
 *
 * Compare LY to LYC if they match and interrupt is enabled, post one.
 *
 */
void Ppu::check_lyc()
{
    if (LY == LYC) {
        if ((STAT & STAT_LYC_F) == 0) {
            STAT |= STAT_LYC_F;
            post_irq(PPU_IRQ);
        }
    } else {
        STAT &= ~STAT_LYC_F;
    }
}

/**
 * @brief Entering Mode 0, post interrupt if enabled.
 *
 */
void Ppu::enter_mode0(bool go)
{
    _mode = 0;
    _start = go;
    post_irq(PPU_IRQ);

    /* Activate memory objects that were blocked */
    if (_mem) {
        if (_vbank) {
            _mem->add_slice(&_data1, 0x8000);
            _mem->add_slice(&_map1, 0x9800);
        } else {
            _mem->add_slice(&_data0, 0x8000);
            _mem->add_slice(&_map0, 0x9800);
        }
        _mem->add_slice(&_oam, 0xfe00);
    }
}

/**
 * @brief Entering Mode 1, post interrupt if enabled.
 *
 */
void Ppu::enter_mode1()
{

if (trace_flag) printf("Vblank Pri=%d\n", _obj_pri);
    _mode = 1;
    _start = true;
    /* Generate VBlank interrupts */
    post_irq(PPU_IRQ);
    post_irq(VBLANK_IRQ);
#if 0
    _oam.print_oam();
    _data0.print_map(0);
    _data1.print_map(1);
    printf("Tile Map:\n");
    for (i = 0; i < 128 ; i++) {
        printf("%04x: ", i << 4);
        for (j = 0; j < 16; j ++) {
            printf("%02x:%02x ", _map0.data[(i << 4) + j],
                                 _map1.data[(i << 4) + j]);
        }
        printf("\n");
    }
    printf("\n");
#endif
}

/**
 * @brief Entering Mode 2, post interrupt if enabled.
 *
 */
void Ppu::enter_mode2()
{
    _mode = 2;
    _start = true;
    /* interrupt if we want to know mode switch */
    post_irq(PPU_IRQ);
}

/**
 * @brief Entering Mode 3
 *
 */
void Ppu::enter_mode3()
{
    _mode = 3;
    _start = true;
    post_irq(PPU_IRQ);
//    _oam.print_sort_oam();
}

static int cycle_cnt = 0;


/**
 * @brief Process a single cycle of pixel engine.
 *
 * Depending on state choose how long to wait to switch to next one.
 *
 * If we are in HBlank wait until 456 pixels have been processes.
 */
void Ppu::dot_cycle() {
    if ((LCDC & LCDC_ENABLE) == 0) {
        return;
    } else {
        check_lyc();
        if (_starting != 0) {
            _starting--;
            return;
        }
    }

if (trace_flag) printf("Mode %d LY %d LX %d C %4d %d %d WY %d WX %d SY %d SX %d LCD %02x\n", _mode, LY, LX, cycle_cnt, _dot_clock, _dot_clock / 4, WY, WX, SCY, SCX, LCDC);
    cycle_cnt++;
    _dot_clock++;
    switch(_mode) {
    case 0:   /* HBlank */
              /* If starting new cycle set things up */
              if (_start) {
                  _start = false;
                  /* If window is being displayed, update to next row */
                  if (_wind_flg) {
                      _wline++;
                      if (_wline == 8) {
                          _wrow++;
                          _wline = 0;
                      }
                  }
                  /* Tell memory to do HDMA transfer */
                  if (_color) {
                      _mem->hdma_cycle();
                  }
              }
              LX++;
              if (LY == 0 && LX == 77) {
                  /* Scan OAM for 10 objects */
                  _oam.scan_oam(16, LCDC, _obj_pri);
                  enter_mode3();
                  break;
              }
              if (_dot_clock == 452) {
                  LY++;
                  check_lyc();
//if (trace_flag) printf("LY %d LYC %d %02x\n", LY, LYC, STAT);
              }
              if (_dot_clock >= 456) {
                  /* Bump to next line */
                  if (LY < 144) {
                      /* Set mode to 2 */
                      enter_mode2();
                  } else {
                      /* Set mode to 1 */
                      enter_mode1();
                  }
                  _dot_clock = 0;
              }
              break;

    case 1:   /* VBlank */
              /* When in vertical blanking we wait until hit bottom of screen*/
              if (_start) {
                  _start = false;
                  /* Draw current screen */
                  draw_screen();
              }
              /* Wait until line finished */
              if (_dot_clock == 452) {
                  LY++;
                  check_lyc();
//if (trace_flag) printf("LY %d LYC %d %02x\n", LY, LYC, STAT);
              }
              if (_dot_clock >= 456) {
                  if (LY >= 154) {
                     /* Set mode to 2 */
                     enter_mode2();
                     _wrow = 0;
                     _wline = 0;
                     _wind_en = false;
                     _wind_flg = false;
                      cycle_cnt = 0;
                     LY = 0;
                     LX = 0;
                     check_lyc();
                     init_screen();
                  }
                  _dot_clock = 0;
              }
              break;
    case 2:   /* OAM Scan */
              if (_start) {
                  _start = false;
                  /* Disable access to OAM memory */
                  if (_mem) {
                      _mem->free_slice(0xfe00);
                  }
              }
              /* Wait until 80 pixels have gone by */
              if (_dot_clock == 80) {
                  /* Scan OAM for 10 objects */
                  _oam.scan_oam(LY+16, LCDC, _obj_pri);
                  /* Set mode to 3 */
                  enter_mode3();
              }
              break;
    case 3:   /* Display */
              /* Start a display cycle. */
              if (_start) {
                  _start = false;
                  /* Disable access to Tile maps and Tile data */
                  if (_mem) {
                      _mem->free_slice(0x8000);
                      _mem->free_slice(0x9800);
                      _mem->free_slice(0xfe00);
                  }
                  LX = 0;
                  /* Initialize the display system */
                  display_start();
              } else {
                  /* Process 1 pixel */
                  display_pixel();
              }
              break;
    }
}

/**
 * @brief Fill pixel FIFO with next 8 pixels from tile.
 *
 * Load the tile into the FIFO at starting pixel. Pix_count is set
 * to the last pixel loaded.
 *
 * @param tile  Pointer to tile to display, including selection of map.
 * @param row   Current tile row to display.
 */
void Ppu::fill_pix(int tile, int row) {
    uint8_t  tnum;
    int      ptr;
    int      i;
    uint8_t  attr = 0;
    bool     flip = false;
    uint8_t  pal = 0;

    /* Load tile map data */
    tnum = _map0.data[tile];
    if ((_ppu_mode & 0xc) == 0) {
        attr = _map1.data[tile];
        pal = (attr & 7) << 2;
        pal |= (attr & 0x80);
        flip = (attr & 0x20) != 0;
    }
    /* Compute index of data */
    if ((LCDC & TILE_AREA) != 0) {
        ptr = (int)tnum;
    } else {
        ptr = 0x100 + (int)((int8_t)tnum);
    }
    /* Multiply by 8 and add in row */
    ptr <<= 3;
    if ((attr & 0x40) != 0) {
        ptr += (row ^ 7);
    } else {
        ptr += row;
    }
if (trace_flag) printf("BG %03x %d %02x %02x %03x %02x:", tile, row, tnum, attr, ptr, LCDC);
    /* Copy row in place */
    for (i = 0; i < 8; i++) {
         int    p = (flip)?(7 - i) : i;
         if ((attr & 0x8) != 0) {
             _pix_fifo[i] = _data1.tile[ptr][p] | pal;
         } else {
             _pix_fifo[i] = _data0.tile[ptr][p] | pal;
         }
if (trace_flag) printf(" %02x", _pix_fifo[i]);
    }
    _pix_count = 7;
if (trace_flag) printf("\n");
}

/**
 * @brief Shift Pixel and Object FIFO left one pixel.
 *
 * Pixel and Object fifo are shifted left one pixel. Transparent
 * pixels are loaded in last location. Pixel count is decremented by
 * one.
 *
 */
void Ppu::shift_fifo() {
    int i;

    /* Shift pixel fifo */
    if (LX >= 8) {
        for (i = 0; i < 7; i++) {
             _pix_fifo[i] = _pix_fifo[i+1];
        }
        _pix_fifo[7] = 0;
        _pix_count--;
    }

    /* Shift obj fifo */
    for (i = 0; i < 7; i++) {
         _obj_fifo[i] = _obj_fifo[i+1];
    }

    /* Put empty pixel in end */
    _obj_fifo[7] = 0;
}

/**
 * @brief Setup to start display of a line.
 *
 * First step is to decide is the Window able to be displayed or
 * not. Next set up a pointer to the current Window and display
 * tile to start the display on. LX is set to the lower 3 bits of SCX.
 * Then clear the fifo's. If background is enabled, load the first
 * file into pixel 8. Then shift the pixel fifo left until LX = 0.
 */
void Ppu::display_start() {
   int     i;

   _obj_num = 0;
   /* Check if window can be displayed */
   _wind_flg = false;
   /* Check if on valid line */
   if (LY == WY) {
       _wind_en = true;
   }
   /* Compute correct tile to start window on */
   _wtile = _wrow << 5;
   /* Compute start of background tile */
   _brow = LY + SCY;
   _btile = (LCDC & BG_AREA) ? 0x400 : 0x000;
   _btile |= (_brow & 0xf8) << 2;
   _btile += (SCX >> 3) & 0x1f;
   _brow &= 0x7;
   LX = SCX & 0x7;
   /* Clear fifos */
   for (i = 0; i < 8; i++) {
       _pix_fifo[i] = 0;
       _obj_fifo[i] = 0;
   }
   _pix_count = 0;

//if (trace_flag) printf("Startline LY %d SCY %d LX %d SCX %d t=%04x\n", LY, SCY, LX, SCX, _btile);
   /* If displaying load first tile */
   if ((LCDC & BG_PRIO) != 0 || (_ppu_mode & 0xc) == 0) {
       fill_pix(_btile | ((LCDC & BG_AREA) ? 0x400 : 0x000), _brow);
       _btile = (_btile & 0x3e0) | ((_btile+1) & 0x1f);
   }

   /* Adjust the tile to start of X */
   while (LX > 0) {
       for (i = 0; i < 7; i++) {
            _pix_fifo[i] = _pix_fifo[i+1];
       }
       _pix_fifo[7] = 0;
       _pix_count--;
       LX--;
   }
   _f_state = GETA;
   _f_type = BG;
   LX = -2;
}

static int mode_0_LX[8] = { 168, 172, 172, 172, 172, 175, 175, 175 };
/**
 * @brief Process one pixel.
 *
 * If we are past 168 pixels we can stop and enter HBlank.
 * Next check if there are only 8 pixels left in pixel Fifo. If so
 * load either the next window tile or backgroud tile into last 8 bits
 * of pixel fifo. Next check if we hit WX and window can be displayed,
 * if so, reset the pixel fifo with first tile of Window.
 *
 * Next look at the current object in OAM, and see if LX matches. If so
 * load that object into the object fifo. We also load in the object palette.
 * When loading only replace pixels with 0 values.
 *
 * Next grab the pixel at head of fifo and see if Object should override it.
 * Pass pixel value to screen to color map and display.
 */
void Ppu::display_pixel() {

    /* Check if into HBlank */
    if (LX >= 168) {
        if (LX >= mode_0_LX[SCX & 0x7]) {
            enter_mode0(true);
        }
        LX++;
        return;
    }

    /* Check if Pixel Fetcher should advance to next state */
    if (_f_state != RDY && _f_type != NONE) {
       switch (_f_state) {
       case INIT:    _f_state = GETA; break;
       case GETA:    _f_state = GETB; break;
       case GETB:    _f_state = DATALA; break;
       case DATALA:  _f_state = DATALB; break;
       case DATALB:  _f_state = DATAHA; break;
       case DATAHA:  _f_state = DATAHB; break;
       case DATAHB:  _f_state = RDY; break;
       case RDY:     _f_state = RDY; break;
       }
    }

    if (LX < 0) {
        LX++;
        return;
    }

    /* Check if we need to insert a next tile */
    if (_pix_count == -1) {

        /* If displaying window, grab from window space */
        if (_wind_flg) {
           /* Check if fetching right type of tile */
           if (_f_type != WIN) {
               _f_state = GETA;   /* No, start fresh */
               _f_type = WIN;
               return;
           }
           /* Wait until ready */
           if (_f_state != RDY) {
               return;
           }
           fill_pix(_wtile | ((LCDC & WIND_AREA) ? 0x400 : 0x000), _wline);
           _wtile = (_wtile & 0x3e0) | ((_wtile+1) & 0x1f);
        } else {
           /* Else if background, grab from background space */
           /* Check if fetching right type of tile */
           if (_f_type != BG) {
               _f_state = GETA; /* No, start fresh cycle */
               _f_type = BG;
               return;
           }
           /* Wait until ready */
           if (_f_state != RDY) {
               return;
           }
           if ((LCDC & BG_PRIO) != 0 || (_ppu_mode & 0xc) == 0) {
               fill_pix(_btile | ((LCDC & BG_AREA) ? 0x400 : 0x000), _brow);
               _btile = (_btile & 0x3e0) | ((_btile+1) & 0x1f);
           }
        }
        /* Reschedule to grab next Background or window */
        _f_state = GETA;
        _f_type = (_wind_flg) ? WIN:BG;
    }

    /* Check to see if we need to switch to window */
    if (_wind_en && !_wind_flg && ((LX-1) == WX)) {
        bool        flg;
        /* Check if window can be displayed */
        if ((_ppu_mode & 0xc) != 0) {
            flg = (LCDC & (WIND_ENABLE|BG_PRIO)) == (WIND_ENABLE|BG_PRIO);
        } else {
            flg = (LCDC & (WIND_ENABLE)) != 0;
        }
        if (flg) {
            /* Check if there is data for us */
            if (_f_type != WIN) {
                _f_state = GETA;  /* Nope, start fresh */
                _f_type = WIN;
                return;
            }
            /* Wait until data ready */
            if (_f_state != RDY) {
                return;
            }
            _wind_flg = flg;
            /* Set for display of Window and load Window into front of Fifo */
            fill_pix(_wtile | ((LCDC & WIND_AREA) ? 0x400 : 0x000), _wline);
            _wtile = (_wtile & 0x7e0) | ((_wtile+1) & 0x1f);
            /* Reschedule to grab next Window */
            _f_state = GETA;
            _f_type = WIN;
        }
    }

    /* Check if we need to load an Object */
    if (_obj_num < 10 && _oam.objs[_obj_num].X == LX) {

        /* Try and fetch a tile */
        if (_f_type != OBJ) {
            _f_state = INIT;
            _f_type = OBJ;
            return;
        }
        /* Wait until ready with data */
        if (_f_state != RDY) {
            return;
        }

        int        mask = (LCDC & OBJ_SIZE) ? 0x7f0: 0x7f8; /* Mask for hieght */
        uint8_t    tile = _oam.objs[_obj_num].tile;
        uint16_t   row = ((uint16_t)tile << 3) & mask;
        uint8_t    flags = _oam.objs[_obj_num].flags;
        bool       overwrite = false;

        /* Set base palette and priority and whether fliped */
        uint8_t    base = ((flags & OAM_PAL) ? 0x8:0x4) |
                          ((flags & OAM_BG_PRI) ? 0x80:0x0);
        bool       flip = (flags & OAM_X_FLIP) != 0;

        /* Adjust base pixels for color support */
        if (_color) {
            base = ((flags & OAM_PAL) ? 0x24:0x20) |
                          ((flags & OAM_BG_PRI) ? 0x80:0x0);
        }

        /* For color, update palette selection */
        if ((_ppu_mode & 0xc) == 0) {
            /* Base is palette, object type and OAM priority */
            base = ((flags & OAM_CPAL) << 2) | 0x60 |
                    ((flags & OAM_BG_PRI) ? 0x80:0x0);
        } else {
            flags &= ~OAM_BANK;
        }

        /* Figure out whether to overwrite object or not */
        if (_obj_pri == 0 || ((_ppu_mode & 0xc) == 0 && _obj_num > 1 &&
                      _oam.objs[_obj_num-1].num < _oam.objs[_obj_num].num &&
                      (_oam.objs[_obj_num-1].X + 8) < _oam.objs[_obj_num].X) ) {
           overwrite = true;
        }

        /* Compute row to show */
        uint16_t y = (LY + 16) - _oam.objs[_obj_num].Y;
        if ((flags & OAM_Y_FLIP) != 0) {
            uint16_t high = (LCDC & OBJ_SIZE) ? 16: 8;  /* Hieght of objects. */
            y = high - y - 1;
        }
        row += y;
if (trace_flag) printf("Obj %02x y=%d r=%04x m=%04x n=%d f=%02x\n", tile, y, row, mask, _oam.objs[_obj_num].num, flags);

//printf("Data : ");
        /* Load object into obj fifo. */
        for (int i = 0; i < 8; i++) {
            int    p = (flip)?(7 - i) : i;
            uint8_t opix;
            if ((flags & OAM_BANK) != 0) {
                opix = _data1.tile[row][p] | base;
            } else {
                opix = _data0.tile[row][p] | base;
            }
            /* Only update if pixel is transparent, or forced overwrite */
            if ((_obj_fifo[i] & 3) == 0 || (overwrite && (opix & 3) != 0)) {
                _obj_fifo[i] = opix;
            }
//        printf("%2x:%02x ", _obj_fifo[i], opix);
        }
//printf("\n");
        _obj_num++;
        /* If same X coord go fetch it. */
        if (_obj_num < 10 && _oam.objs[_obj_num].X == LX) {
            _f_state = INIT;
            _f_type = OBJ;
            return;
        }
        /* Reschedule to grab next Background or Window */
        _f_state = INIT;
        _f_type = (_wind_flg) ? WIN:BG;
    }

    /* Display next pixel */
    if (LX >= 8) {
        uint8_t   opix = _obj_fifo[0];
        uint8_t   pix = _pix_fifo[0];

        /* Start with pixel fifo */
        if ((_ppu_mode & 0xc) == 0) {
            /* If object are enabled see if Object will override */
            if ((LCDC & OBJ_ENABLE) != 0) {
                 /* If background has priority, only override if object */
                 if ((LCDC & BG_PRIO) == 0) {
                     if ((opix & 3) != 0) {
                        pix = opix;
                     }
                 } else {
                     /* Otherwise if both Pix priority and OAM use object */
                     if ((pix & 0x80) != 0 || (opix & 0x80) != 0) {
                         if ((pix & 3) == 0) {
                            pix = opix;
                         }
                     } else {
                         if ((opix & 3) != 0) {
                             pix = opix;
                         }
                     }
                 }
            }
        } else {
            if ((LCDC & BG_PRIO) == 0) {
               pix = 0;
            }
            /* If object are enabled see if Object will override */
            if ((LCDC & OBJ_ENABLE) != 0) {
                /* background has priority, overwrite if transparent background */
                if ((opix & 0x80) != 0) {
                   if ((pix & 0x3) == 0 && (opix & 3) != 0) {
                       pix = opix;
                   }
                } else {
                   /* Object has priority, overwrite if not transparent */
                   if ((opix & 3) != 0) {
                       pix = opix;
                   }
                }
            }
        }
        pix &= 0x3f;
        /* Display pixel */
        draw_pixel(pix, LY, LX-8);
    }
    /* Shift fifo one pixel left. */
    shift_fifo();

    LX++;
}

/**
 * @brief Set Video Bank.
 *
 * @param data value to set.
 */
void Ppu::set_vbank(uint8_t data) {
     if ((_ppu_mode & 0xC) == 0) {
         _vbank = (int)(data & 1);
         if (_mem && _mode != 3) {
             if (_vbank) {
                 _mem->add_slice(&_data1, 0x8000);
                 _mem->add_slice(&_map1, 0x9800);
             } else {
                 _mem->add_slice(&_data0, 0x8000);
                 _mem->add_slice(&_map0, 0x9800);
             }
         }
     }
}

/**
 * @brief Set Video Mode.
 *
 * Bit 0 - Unknown.
 * Bit 2 - Disable RP(IR), VBK, SVBK, FF6C
 * Bit 3 - Disable HDMA, BGPD, OBPD, and KEY1.
 *
 * @param data value to set.
 */
void Ppu::set_ppu_mode(uint8_t data) {
     if (!_mem->get_disable()) {
         _ppu_mode = data;
         if ((_ppu_mode & 0x8) != 0) {
            _mem->add_slice(&_data0, 0x8000);
            _mem->add_slice(&_map0, 0x9800);
         }
     }
}

/**
 * @brief Read PPU control registers.
 *
 * Read control registers.
 *
 * @param[out] data Data read from register.
 * @param[in]  addr Register to read.
 */
void Ppu::read_reg(uint8_t &data, uint16_t addr) const {
     switch(addr & 0xf) {
     case 0x0: data = LCDC; break;         /* ff40 */
     case 0x1: data = STAT | 0x80;         /* ff41 */
               if ((data & LCDC_ENABLE) != 0) {
                   data |= _mode;
               }
               break;
     case 0x2: data = SCY; break;          /* ff42 */
     case 0x3: data = SCX; break;          /* ff43 */
     case 0x4: data = LY; break;           /* ff44 */
     case 0x5: data = LYC; break;          /* ff45 */
     case 0x6: if (_mem) {
                   _mem->read_dma(data);
               }
               break;                      /* ff46 DMA */
     case 0x7: data = BGP; break;          /* ff47 */
     case 0x8: data = OBP0; break;         /* ff48 */
     case 0x9: data = OBP1; break;         /* ff49 */
     case 0xa: data = WY; break;           /* ff4a */
     case 0xb: data = WX; break;           /* ff4b */
     default:  data = 0xff; break;
     }
}

/**
 * @brief Write PPU control registers.
 *
 * Write control registers.
 *
 * Writing to LCDC register will start displaying if there is
 * change in flag. Writing to the DMA control register tells the
 * Memory object to start a DMA transfer. Writing to the palette
 * registers passes the changes onto the screen object so it can
 * convert them into SDL colors. Other updates just change the value
 * of internal registers.
 *
 * @param[out] data Data write to register.
 * @param[in]  addr Register to write.
 */
void Ppu::write_reg(uint8_t data, uint16_t addr) {
     switch(addr & 0xf) {
     case 0x0:                          /* ff40 */
               /* Check if we are enabling or disabling controller */
               if (((LCDC ^ data) & LCDC_ENABLE) != 0) {
                   if ((data & LCDC_ENABLE) != 0) {
                       LX = LY = 0;
                       check_lyc();
                       enter_mode0(true);
                       _dot_clock = 0;
                       _starting = 2;
                       cycle_cnt = 0;
                   } else {
                       LX = LY = 0;
                       enter_mode0(false);
                       _dot_clock = 0;
                       _starting = 0;
                   }
               }
               LCDC = data;
               break;
     case 0x1: STAT = (data & 0x78) | (STAT & STAT_LYC_F);    /* ff41 */
               post_irq(PPU_IRQ);
               break;
     case 0x2: SCY = data; break;       /* ff42 */
     case 0x3: SCX = data; break;       /* ff43 */
     case 0x4: break;                   /* ff44 LY */
     case 0x5: LYC = data;              /* ff45 */
               if ((data & LCDC_ENABLE) != 0) {
                   check_lyc();
               }
               break;
     case 0x6: if (_mem) {
                   _mem->write_dma(data);
               }
               break;                   /* ff46 DMA */
     case 0x7: BGP = data;              /* ff47 */
               if ((_ppu_mode & 0xc) != 0) {
                   set_palette_bw(0, data, _color);
               }
               break;
     case 0x8: OBP0 = data;             /* ff48 */
               if ((_ppu_mode & 0xc) != 0) {
                   set_palette_bw(0x4, data, _color);
               }
               break;
     case 0x9: OBP1 = data;             /* ff49 */
               if ((_ppu_mode & 0xc) != 0) {
                   set_palette_bw(0x8, data, _color);
               }
               break;
     case 0xa: WY = data; break;       /* ff4a */
     case 0xb: WX = data; break;       /* ff4b */
     default:
               break;
     }
}
