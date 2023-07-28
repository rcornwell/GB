/*
 * GB - Default audio waveforms.
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

#include "Apu.h"

const uint8_t Sound::sq_wave[32] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,   /* 12.5 % */
            0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,   /* 25.0 % */
            0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10,   /* 50.0 % */
            0x00, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00    /* 75.0 % */
};

const uint8_t Sound::int_wave[32] = {
            0x0a, 0x0c, 0x0d, 0x0d, 0x0d, 0x0a, 0x04, 0x08,
            0x03, 0x06, 0x00, 0x02, 0x0c, 0x0f, 0x01, 0x06,
            0x02, 0x0c, 0x00, 0x04, 0x0e, 0x05, 0x02, 0x0c,
            0x0a, 0x0c, 0x0d, 0x0d, 0x0d, 0x0a, 0x04, 0x08,
};
