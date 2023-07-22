/*
 * GB - Joy pad buttons.
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

#define RIGHT 0x10
#define LEFT  0x20
#define UP    0x40
#define DOWN  0x80

#define ABUT  0x01
#define BBUT  0x02
#define SELECT 0x04
#define START 0x08

/**
 * @brief Joy pad Object.
 *
 * The Joypad object handles the buttons.
 * It has two output pins to select which set of 4 buttons
 * to read. Setting either of the output lines to 1 results
 * In 1's for unpushed buttons and 0's for pushed buttons.
 */
class Joypad {
protected:
     uint8_t     *_irq_flg;            /**< Interrupt flag pointer */
     uint8_t     _out_bits;            /**< Current output state */
     uint8_t     _joy_buttons;         /**< Current buttons pressed */

public:

     Joypad () : _irq_flg(NULL), _out_bits(0xc0), _joy_buttons(0) { };

     /**
      * @brief Connect to interrupt register
      *
      * Set pointer to where interrupts need to be updated.
      *
      * @param irq_flg Pointer to interrupt register in CPU.
      */
     virtual void set_irq(uint8_t *irq_flg) {
         _irq_flg = irq_flg;
     }

     /**
      * @brief Post interrupt flag.
      *
      * When there is a change in button state, generate an interrupt.
      * @param value mask of bit to set..
      */
     virtual void post_irq(uint8_t value) {
         *_irq_flg |= value;
     }

     /**
      * @brief Read Joypad bits.
      *
      * Use outbits to decide which half of the register to output.
      * @param[out] data Data read from buttons.
      * @param[in] addr Address to read from, ignored since only one device.
      */
     virtual void read(uint8_t &data, [[maybe_unused]]uint16_t addr) {
         data = _out_bits & 0xF0;

         if ((_out_bits & 0x20) != 0) {
             data |= ((_joy_buttons >> 4) & 0xf);
         }
         if ((_out_bits & 0x10) != 0) {
             data |= (_joy_buttons & 0xf);
         }
         data ^= 0xf;

     }

     /**
      * @brief Write Joypad bits.
      *
      * Set outbits indicated which row to select. 
      * @param[in] data Data to write to outbits.
      * @param[in] addr Address to write to, ignored since only one device.
      */
     virtual void write(uint8_t data, uint16_t addr) {
         _out_bits = data | 0xc0;
     }

     /**
      * @brief Press buttons.
      *
      * Called by SDL polling routine when a key is pressed. Sets the bit for 
      * the button in joy_buttons for tracking. If outbits selects any pressed
      * buttons generate an interrupt.
      * @param button Mask of button pressed.
      */
     void press_button(uint8_t button) {
         uint8_t   f = 0;

         _joy_buttons |= button;
         if ((_out_bits & 0x20) != 0) {
            f |= !(button & 0xf0);
         }
         if ((_out_bits & 0x10) != 0) {
            f |= !(button & 0xf);
         }

         if (f) {
             *_irq_flg |= 0x10;
         }
     }

     /**
      * @brief Release buttons.
      *
      * Called by SDL polling routine when a key is released. Clears the bit in 
      * the current button register.
      * @param button Mask of button pressed.
      */
     void release_button(uint8_t button) {
         _joy_buttons &= ~button;
     }
};

