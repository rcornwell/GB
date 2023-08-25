/*
 * GB - Timer
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
#include "Device.h"
#include "Apu.h"
#include "Cartridge.h"

/**
 * @brief Interval timer object.
 *
 * Provide regular interrupts to the game system.
 */
class Timer : public Device {
     uint16_t    _div;              /**< Divider register */
     uint16_t    _tima;             /**< Current timer register */
     uint8_t     _tma;              /**< Timer reload register */
     uint8_t     _tac;              /**< Timer control register */
     bool        _time_over;        /**< Timer overflowed */
     Apu        *_apu;              /**< Apu since timer needs to
                                         be able to give clocks to
                                         the APU */
     Cartridge  *_cart;             /**< Send cartridge a 1hz signal */
     int32_t     _sec_time;         /**< Second count down */
     bool        _speed;            /**< Running at double speed */
     uint16_t    _apu_mask;         /**< Mask for APU bit to monitor */

     /**< Mask indicating bits that cause interrupt */
     const uint16_t t_mask[4] = {
            0x0200, 0x0008, 0x0020, 0x0080
     };

public:
     Timer() : _apu(NULL), _cart(NULL) {
        _tima = _tma = _tac = 0;
        _time_over = false;
        _div = 8;
        _speed = false;
        _apu_mask = 0x1000;
        _sec_time = 2 * 1024 * 1024;
     }

     /**
      * @brief Address of APU unit.
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x04;
     }

     /**
      * @brief Number of registers APU unit has.
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 4;
     }

     /**
      * @brief Connect to APU to provide low frequency timing.
      *
      * Every 512 cycles we call the APU to do low speed updates.
      */
     void set_apu(Apu *apu) {
          _apu = apu;
     }

     /**
      * @brief Connect to Cartridge to provide 1 second ticks if needed.
      *
      * Give the cartridge a 1 second clock.
      */
     void set_cart(Cartridge *cart) {
          _cart = cart;
     }

     /**
      * @brief Set speed.
      *
      * @param speed New speed.
      */
     void set_speed(bool speed) {
         _apu_mask = (speed) ? 0x2000 : 0x1000;
         _speed = speed;
     }

     /**
      * @brief Cycle timer.
      *
      * Called once per cycle by Memory controller.
      * Mask points to the bit in the DIV register we need to check.
      * Also every 512 cycles we need to call the APU.
      */
     virtual void cycle() override {
          uint16_t  prev     = !!(_div & t_mask[_tac&3]);
          uint16_t  prev_snd = !!(_div & _apu_mask);

          _div += 4;
          _time_over = false;
//if (trace_flag) printf("Div %04x TIMA %02x TMA %02x %d\n", _div, _tima, _tma, prev);
          if ((_tac & 0x4) != 0) {
             if ((_tima & 0x100) != 0) {
                 /* Generate timer interrupt */
                 post_irq(TIMER_IRQ);
                 /* Reload tima */
                 _tima = (uint16_t)_tma;
                 _time_over = true;
             } else {
                 /* Update tima if div bit changed from 1 to 0 */
                 if (prev != 0 && (_div & t_mask[_tac&3]) == 0) {
                     _tima++;
                 }
             }
          }
          /* Update sound state 512 times per second */
          if (_apu != NULL && prev_snd != 0 && (_div & _apu_mask) == 0) {
              _apu->cycle_sound();
          }
          /* Update second clock, decrement by -1 or -2 depending on speed setting. */
          _sec_time -= 1 + !_speed;
          if (_sec_time <= 0) {
             _sec_time += 2 * 1024 * 1024;
             _cart->tick();
          }
     }

     /**
      * @brief Read timer registers.
      *
      * Read the timer registers.
      *
      * @param[out] data Value read from register.
      * @param[in] addr Address of register.
      */
     virtual void read_reg(uint8_t &data, uint16_t addr) const override {
          switch(addr & 0x3) {
          case 0: data = (_div >> 8) & 0xff; break;      /* ff04 */
          case 1: data = (uint8_t)(_tima & 0xff); break; /* ff05 */
          case 2: data = _tma & 0xff; break;             /* ff06 */
          case 3: data = _tac | 0xf8; break;             /* ff07 */
          }
     }

     /**
      * @brief Write timer registers.
      *
      * Update the timer registers. Writing to DIV will always
      * reset it. If the timer was previously enabled and the new
      * state is enabled, check if the frequency selection bit
      * changed from 1 to 0, if so trigger a timer update.
      *
      * @param data Data to write to register.
      * @param addr Address of register to write.
      */
     virtual void write_reg(uint8_t data, uint16_t addr) override {
          switch(addr & 0x3) {
          case 0:                        /* ff04 */
                   if ((_tac & 0x4) != 0) {
                       uint16_t   prev = !!(_div & t_mask[_tac&3]);

                       /* If new value is enabled trigger count if change */
                       if (prev) {
                           _tima++;
                       }
                   }
                   _div = 0;
                   break;
          case 1: /* If tima is updated on same cycle it overflows,
                     grab tma rather then new data */
                  if (_time_over) {
                      _tima = _tma;
                  } else {
                      _tima = data;
                  }
                  break;                  /* ff05 */
          case 2: _tma = data;
                  /* If we overflowed on this cycle, update tima also */
                  if (_time_over) {
                      _tima = data;
                  }
                  break;                 /* ff06 */
          case 3:                        /* ff07 */
                  /* If timer was previously enabled check value of div bit */
                  if ((_tac & 0x4) != 0 /*&& (data & 0x4) != 0*/) {
                      uint16_t   prev = !!(_div & t_mask[_tac&3]);
                      uint16_t   next = !!(_div & t_mask[data&3]);

                      /* Check if we might get glitch trigger */
                      if ((data & 0x4) == 0 && prev != 0) {
                          _tima++;
                      }
                      /* If new value is enabled trigger count if change */
                      if ((data & 0x4) != 0 && prev != next && next == 0) {
                          _tima++;
                      }
                      if ((_tima & 0x100) != 0) {
                          /* Generate timer interrupt */
                          post_irq(TIMER_IRQ);
                          /* Reload tima */
                          _tima = _tma;
                          _time_over = true;
                      }
                  }
                  _tac = data;
                  break;
          }
     }

};
