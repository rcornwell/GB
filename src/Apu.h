/*
 * GB - Audio Processing Unit
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
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "Device.h"
#include "System.h"

/**
 * @brief Default Sound channel object.
 *
 * @class Sound Apu.h "Apu.h"
 *
 * All Game Boy sound channels have the same basic functions.
 * They start with a frequency divider (or noise generator for
 * channel 4). Whenever this divider counts down to zero the next
 * sample is generated, the counter is also reloaded with the
 * initial frequency setting. The counter is clocked off either the
 * dot clock / 2 or by 4.
 *
 * Each channel also has a length counter which is used for decremented
 * at a rate of 256hz. When this counter reaches zero audio output stops
 * If the sound generator is started without using a length the counter
 * is loaded with the max value.
 *
 * Each channel also has a volume envelop generator. This will either
 * increase of decrease the volume at a rate of 64hz. When the volume
 * reaches 15 or 0 the envelop stops operating.
 *
 * Channel 1 has a frequency sweep function which adjusts the frequency
 * counter at a rate of 128hz. The frequency can be shifted up or down
 * by a fractional amount each frame.
 */
class Sound {
protected:
    uint8_t    _wave[32];      /**< Pointer to wave data */
    uint16_t   _freq_cnt;      /**< Frequency counter */
    int        _wave_start;    /**< Where to start wave playback */
    int        _wave_end;      /**< End of wave playback */
    uint16_t   _int_freq;      /**< Initial frequency counter */
    int        _count;         /**< Current frequency counter */
    int        _length;        /**< Length in 1/64th of a second */
    int        _max_length;    /**< Max length value */
    int        _int_len;       /**< Initial value of length */
    bool       _use_len;       /**< Use length value */
    int        _pos;           /**< Current position in playback */
    uint8_t    _volume;        /**< Current output volume */
    uint8_t    _int_vol;       /**< Initial volume */
    uint8_t    _int_vol_len;   /**< Initial volume length */
    int        _vol_sweep;     /**< Volume sweep */
    int        _vol_dir;       /**< Sweep direction */
    int        _vol_len;       /**< Length of volume envelop */
    bool       _vol_env;       /**< Do volume envelop */
    uint8_t    _duty;          /**< Duty cycle */
    uint8_t    _chan;          /**< Channel number */
    bool       _dac_enable;    /**< Channel's DAC is disabled */
    bool       _delay;         /**< One cycle delay after trigger */

    const static uint8_t sq_wave[32];
    const static uint8_t int_wave[32];
public:
    bool       enabled;     /**< Channel enabled */
    bool       active;      /**< Sound currently playing */
    int8_t     sample;      /**< Current output value */

    Sound() : enabled(false), active(false), sample(0) {
         _freq_cnt = _int_freq = 0;
         _pos = _count = _length = _int_len = 0;
         _volume = _int_vol = _int_vol_len = 0;
         _vol_sweep = _vol_dir = _vol_len = _duty = 0;
         _dac_enable = false;
         _delay = _use_len = _vol_env = false;
         _chan = 0;
         _wave_start = 0;
         _wave_end = 8;
         _max_length = 64;
         for (int i = 0; i < 32; i++) {
             _wave[i] = sq_wave[i];
         }
    }

    /**
     * @brief Start playing a sound.
     *
     * Set initial frequency, length and volume.
     */
     virtual void start(bool trigger,
                        bool prev_use_len, bool next_frame_length);

    /**
     * @brief reset channel.
     *
     * Stops any channel activity and set it to default state.
     */
    virtual void reset();

    /**
     * @brief Update channels length counter.
     *
     * Clock up length counter until it overflows, then stop channel.
     * If not enabled or not using length, do nothing.
     */
    virtual void update_length();

    /**
     * @brief Update channels volume sweep.
     */
    virtual void update_volume();

    /**
     * @brief Update channels frequency  sweep.
     *
     * Called to update the frequency at frequency sweep time. This is
     * only defined for channel 1.
     */
    virtual void update_sweep();

    /**
     * @brief Update frequency sequence counter for one clock pulse.
     *
     * We count the timer up until it overflows. At which time we select the
     * next wave sample and advance the sample, then we reset the timer.
     * Otherwise just wait until it overflows.
     */
    virtual void cycle();

    /**
     * @brief read register 0.
     *
     * Frequency sweep shift register.
     */
    virtual void read_reg0(uint8_t& data) const {
        data = 0xff;
    }

    /**
     * @brief read register 1.
     *
     * Sound length register.
     */
    virtual void read_reg1(uint8_t& data) const {
        data = (_duty << 6) | 0x3f;
    }

    /**
     * @brief read register 2.
     *
     * Volume envelop register.
     */
    virtual void read_reg2(uint8_t& data) const {
        data = (_int_vol << 4) | (_vol_dir ? 0x8:0x0) | _int_vol_len;
    }

    /**
     * @brief read register 3.
     *
     * Frequency register lower.
     */
    virtual void read_reg3(uint8_t& data) const {
        data = 0xff;
    }

    /**
     * @brief read register 4.
     *
     * Frequency register upper.
     */
    virtual void read_reg4(uint8_t& data) const {
        data = (_use_len) ? 0xff : 0xbf;
    }

    /**
     * @brief write register 0.
     *
     * Frequency sweep shift register.
     */
    virtual void write_reg0(uint8_t data);

    /**
     * @brief write register 1.
     *
     * Sound length register.
     */
    virtual void write_reg1(uint8_t data);

    /**
     * @brief write register 2.
     *
     * Volume envelop register.
     */
    virtual void write_reg2(uint8_t data);

    /**
     * @brief write register 3.
     *
     * Frequency register lower.
     */
    virtual void write_reg3(uint8_t data);

    /**
     * @brief write register 4.
     *
     * Frequency register upper.
     */
    virtual void write_reg4(uint8_t data, int next_length);
};

/**
 * @brief Sound Channel 2.
 *
 * @class S2 Apu.h "Apu.h"
 *
 * Sound channel 2 is the default channel. It plays a fixed wave of
 * 8 samples. These are set to play a square wave. It lacks the
 * Frequency sweep function.
 */
class S2 : public Sound {
public:
     S2() {
          /* Initialize wave table */
          _chan = 2;
     }

};

/**
 * @brief Sound Channel 1.
 *
 * @class S1 Apu.h "Apu.h"
 *
 * Sound channel 1 is the same as Sound Channel 2, but includes
 * a frequency sweep function. It also plays an 8 samples. These
 * are set to play a square wave.
 */
class S1 : public S2 {
     uint16_t  _sweep_freq;   /**< Sweep frequency */
     int       _sweep_period; /**< Frequency sweep interval */
     int       _sweep_clk;    /**< Frequency sweep clock */
     bool      _sweep_dir;    /**< Direction of frequency sweep */
     int       _sweep_shift;  /**< Number of steps */
     bool      _shift_ena;    /**< Enable frequency shift */
     bool      _last_sub;     /**< Last update was subtract */
     uint8_t   _reg0;         /**< Backup copy of register 0 */
public:

     S1() : S2() {
          _chan = 1;
          _sweep_freq = 0;
          _reg0 = 0;
          _sweep_period = _sweep_shift = _sweep_clk = 0;
          _sweep_dir = _shift_ena = _last_sub = false;
     }


     /**
      * @brief Calculate the next frequency with sweep.
      *
      * Depending on the value of _freq_dir either increment or
      * decrement the frequency.
      *
      * @return Then next frequency.
      */
     uint16_t next_freq();

     /**
      * @brief Update frequency.
      *
      * When update_sweep times out update frequency to new one.
      *
      * We do this by shifting the initial frequency right by
      * shift _freq_shift times, we then add or subtract this
      * value from initial frequency. If we overflow or under
      * flow set frequency to zero and stop the playback.
      */
     void update_step();

     /**
      * @brief Update channels frequency  sweep.
      */
     virtual void update_sweep() override;

     /**
      * @brief read register 0.
      *
      * Frequency sweep shift register.
      */
     virtual void read_reg0(uint8_t& data) const override {
         data = _reg0 | 0x80;
     }


     /**
      * @brief write register 0.
      *
      * Frequency sweep shift register.
      */
     virtual void write_reg0(uint8_t data) override;

     /**
      * @brief write register 4.
      *
      * Frequency register upper.
      *
      * When next step does not change Length counter, if previously disabled
      * and now enabled and counter != 0, decrement. If length == 0, disable
      * channel.
      *
      * When triggered if length == 0, set to max length.
      *
      * When triggering, and next does not change length counter, and length
      * is now enabled and length is being set to max length, decrement counter.
      */
     virtual void write_reg4(uint8_t data, int next_length) override;
};

/**
 * @brief Sound Channel 3
 *
 * @class S3 Apu.h "Apu.h"
 *
 * Sound Channel 3 is the same as sound channel 2, only it
 * plays a 32 sample wave which can be loaded by the CPU. It also
 * lacks a volume envelop.
 */
class S3 : public Sound {
     bool     _chan_ena;      /**< Indicates if channel is enabled or not */
     uint8_t  _last_read;     /**< Last wave table read sample */
     uint8_t  _out_vol;       /**< Output volume */
public:
     S3() {
          _chan = 3;
          _chan_ena = false;
          _wave_start = 0;
          _wave_end = 32;
          _duty = 3;
          _max_length = 256;
          _out_vol = 0;
          _last_read = 0xff;
          /* Initialize wave table */
          for (int i = 0; i < 32; i++) {
              _wave[i] = int_wave[i];
          }
     }

     /**
      * @brief Update channels volume sweep.
      */
     virtual void update_volume() override {
     }

     /**
      * @brief Update frequency counter for one clock pulse.
      *
      * We count the timer up until it overflows. At which time we select the
      * next wave sample and advance the sample, then we reset the timer.
      * Otherwise just wait until it overflows.
      */
     void early_cycle() {
         if (!_delay) {
             cycle();
         }
     }

     /**
      * @brief Update wave buffer for read of wave memory.
      */
     virtual void wave_cycle() {
         if (active) {
             _last_read = 0xff;
             if (_freq_cnt == 0x7ff) {
                  int pos = (_pos) & 0x1e;
                  _last_read = (_wave[pos] << 4);
                  _last_read |= _wave[pos | 1];
             }
         }
     }

     /**
      * @brief read register 0.
      *
      * Frequency sweep shift register.
      */
     virtual void read_reg0(uint8_t& data) const override {
         data = (_dac_enable) ? 0xff : 0x7f;
     }

     /**
      * @brief read register 2.
      *
      * Output volume.
      */
     virtual void read_reg2(uint8_t& data) const override {
         data = (_out_vol << 5) | 0x9f;
     }

     /**
      * @brief write register 0.
      *
      * Frequency sweep shift register.
      */
     virtual void write_reg0(uint8_t data) override;

     /**
      * @brief write register 1.
      *
      * Sound length register.
      */
     virtual void write_reg1(uint8_t data) override;

     /**
      * @brief write register 2.
      *
      * Output volume
      */
     virtual void write_reg2(uint8_t data) override;

     /**
      * @brief read wave register.
      *
      * The wave register is store expanded to two samples, so
      * they have to be combined in read function.
      *
      * param[out] data   Value of wave at index.
      * param[in]  addr   Index to read.
      */
      void read_wave(uint8_t& data, uint16_t index) const;

     /**
      * @brief write wave register.
      *
      * The wave register is store expanded to two samples, so
      * they have to be split apart and shifted for writing.
      *
      * param[out] data   Value of wave at index.
      * param[in]  addr   Index to read.
      */
      void write_wave(uint8_t data, uint16_t index);
};

/**
 * @brief Sound Channel 4
 *
 * @class S4 Apu.h "Apu.h"
 *
 * Sound Channel 4 lacks a frequency generator or a wave table. It
 * outputs a pseudo random stream to simulate noise (LSFR). Instead of a
 * frequency divider, channel 4 has a simple divider the controls
 * when the LSFR should generate a new sample. The LSFR is a shift
 * register that is initialized to a random value, and each time it is
 * clocked shifts one bit right. The two high order bits are exclusive
 * or'd together and sent back into bit 0. The LSFR can be configured
 * as either 15 or 7 bits. It is implemented as a 16 bit register. The
 * output is always the high bit, and the highest two bits are feed
 * back into the shift register.
 */
class S4 : public Sound {
     uint16_t        _shift_reg;   /**< Random noise register */
     int             _div_ratio;   /**< Divider ratio */
     int             _steps;       /**< 7 or 15 bit LSFR */
     int             _clk_freq;    /**< Clock frequency */
     int             _div;         /**< Freq divide counter */
     uint16_t        _clk_cnt;     /**< Shift counter */
     uint8_t         _reg3;        /**< Saved copy of NR43 */
public:

     S4() {
          _chan = 4;
          _div_ratio = _steps = _clk_freq = _div = 0;
          _clk_cnt = 0;
          _duty = 3;
          _reg3 = 0;
          std::srand((unsigned int)std::time(nullptr));
          int seed = std::rand();

          _shift_reg = (seed >> 10) & 0x7fff;
     }

     /**
      * @brief reset channel.
      *
      * Stops any channel activity and set it to default state.
      */
     virtual void reset() override;

     /**
      * @brief simulate the noise shift register.
      *
      * The shift register is either 15 or 7 bits long. We simulate this by a
      * 16 bit register. Each step the shift register is shifted 1 bit to the
      * left. The top two bits (15,14) or (7,6) are compared and fed into lowest
      * bit of shift register. Bit 15 or 7 is taken as the output.
      */
     void shift();

     /**
      * @brief Update frequency counter for one clock pulse.
      *
      * If the divide ratio reaches zero. We reset the divide ratio, then if
      * the polynomial counter clock reached zero we generate and output and
      * reset the counter clock to 2^n.
      */
     virtual void cycle() override;

     /**
      * @brief read register 1.
      *
      * Sound length register.
      */
     virtual void read_reg1(uint8_t& data) const override {
         data = 0xff;
     }

     /**
      * @brief read register 3.
      *
      * Frequency register lower.
      */
     virtual void read_reg3(uint8_t& data) const override {
         data = _reg3;
     }

     /**
      * @brief write register 1.
      *
      * Sound length register.
      */
     virtual void write_reg1(uint8_t data) override;

     /**
      * @brief write register 3.
      *
      * Divider register.
      */
     virtual void write_reg3(uint8_t data) override;

     /**
      * @brief read register 4.
      *
      * Frequency register upper.
      */
     virtual void write_reg4(uint8_t data, int next_length) override;
};

/**
 * @brief Audio Processing Unit.
 *
 * @class Apu Apu.h "Apu.h"
 *
 * The audio processing unit generates all the sounds for the
 * Game Boy. It is clocked off the main system clock and from
 * the DIV clock. If the DIV register is written this can change
 * how timing will occur.
 *
 * @verbatim
 * NR10    Bit 7        not used.
 *         Bit 6-4      Sweep time ts.
 *            000:  Sweep off.
 *            001:  ts=1/f128 =  7.8ms   8176
 *            010:  ts=2/f128 = 15.6ms  16352
 *            011:  ts=3/f128 = 23.4ms  24528
 *            100:  ts=4/f128 = 31.3ms  32809
 *            101:  ts=5/f128 = 39.1ms  40986
 *            110:  ts=6/f128 = 46.9ms  49161
 *            111:  ts=7/f128 = 54.7ms  57337
 *         Bit 3        Sweep Increase/Decrease.
 *         Bit 2-0      Sweep Shift Number 0-7
 *
 * NR11    Bit 7-6      Bit 7 output, 6 Write only
 *             00:   12.5%
 *             01:   25%
 *             10:   50%
 *             11:   75%
 *         Bit 5-0:     Sound length t1(0 to 63).
 *
 * NR12    Bit 7-4      Default Envelope Value
 *         Bit 3        Envelop Down/Up
 *         Bit 2-0      Length of step n(0 to 7).
 *                          Nx1/64 sec.
 *
 * NR13    Bit 7-0      Low order freq (Write only)
 *
 * NR14    Bit 7        Initialize. (Write only)
 *         Bit 6        Counter Continuous Selection
 *         Bit 5-3      Unused.
 *         Bit 2-0      High order freq (Write only)
 *
 *         f = 4194304/(4x2x(2048-X)) HZ.
 *               Min = 64Hz, 131.1kHz
 *
 * NR21-24  Same as NR11-NR14
 *
 * NR30    Bit 7      Stop/Enable sound. (R/W)
 *         Bit 6-0    Unused
 *
 * NR31    Bit 7-3     Length of sound (0-255)
 *         Bit 2-0     Unused.
 (                 Length = (256-t1)x(1/256) sec
 *
 * NR32    Bit 7      unused.
 *         Bit 6-5    Output level.
 *              00     Mute
 *              01     4 bit.
 *              10     shift left 1. 1/2
 *              11     shift left 2. 1/4
 * NR33    Bit 7-0     Low order freq (write only)
 *
 * NR34    Bit 7       Initialize.
 *         Bit 6       Counter/continuous.
 *                     0 continuous.
 *                     1 length NR31. When done bit2 NR52 Sound 3 on reset.
 *
 * NR41    Bit 7-6     Unused.
 *         Bit 5-0     Sound length t1(0 to 63).
 *
 * NR42    Same NR12
 *
 * NR43     Bit 7-4    Selects Shift Clock.
 *              000:   fx1/2^3x2
 *              001:   fx1/2^3x1
 *              010:   fx1/2^3x1/2
 *              011:   fx1/2^2x1/3
 *              100:   fx1/2^2x1/4
 *              101:   fx1/2^2x1/5
 *              110:   fx1/2^2x1/6
 *              111:   fx1/2^2x1/7
 *          Bit 3      Selects number of ppolynomial0=15,1=7
 *          Bit 2-0    Division ratio ffrequency
 *
 * NR44     Bit 7      Initialize.
 *          Bit 6      Counter/Continuous.
 *
 * NR50     Bit 7      Vin -> S02 on/off
 *          Bit 6-4    SO2 output level 0-7
 *          Bit 3      Vin -> S01 on/off
 *          Bit 2-0    SO1 output level 0-7
 *
 * NR51     Bit 7-4    Output Sound 4-1 to SO2
 *          Bit 3-0    Output Sound 4-1 to S01
 *
 * NR52     Bit 7      Enable sounds.
 *          Bit 3-0    Sound 4-1 on flag.
 *
 * @endverbatim
 *
 */
class Apu : public Device {
     uint8_t      regs[2];         /**< NR5x registers */
     int          _fr_counter;     /**< Frame counter */
     int          _sample_cnt;     /**< Sample counter */
     bool         _enabled;        /**< Sound system enabled */

public:
     S1           s1;              /**< Sound channel 1 */
     S2           s2;              /**< Sound channel 2 */
     S3           s3;              /**< Sound channel 3 */
     S4           s4;              /**< Sound channel 4 */

     uint8_t      SO1;             /**< Sound output S1 Right */
     uint8_t      SO2;             /**< Sound output S2 Left */

     Apu() {
        _fr_counter = 0;
        _sample_cnt = 0;
        _enabled = false;
        regs[0] = 0;
        regs[1] = 0;
        SO1 = SO2 = 0;
     }

    /**
     * @brief Process a single cycle, early.
     *
     */
    void cycle_early() {
       s3.early_cycle();
    }

    /**
     * @brief Process a single cycle.
     *
     * Every 32 cycles sample the output of each channel. Then call
     * each channels cycle function to update their frequency count.
     */
    void cycle() override;

    /**
     * @brief Called 512 times per second to update the sequencer.
     *
     * We need to call update length every other cycle. Every 4
     * cycles we call update sweep.
     *
     */
    void cycle_sound();

    /**
      * @brief Address of APU unit.
      *
      * @return base address of device.
      */
     virtual uint8_t reg_base() const override {
         return 0x10;
     }

     /**
      * @brief Number of registers APU unit has.
      *
      * @return number of registers.
      */
     virtual int reg_size() const override {
         return 48;
     }

    /**
     * @brief Audio Processor Unit register read.
     *
     * Read channel registers. Each channel implements
     * an individual read for each register. By use
     * of virtual functions channels can return the
     * correct value along with any stuck bits.
     *
     * @param[out] data Data read from register.
     * @param[in] addr Address of register to read.
     */
    virtual void read_reg(uint8_t &data, uint16_t addr) const override;

    /**
     * @brief Audio Processor Unit register write.
     *
     * Write channel registers. Each channel implements
     * an individual write for each register. By use
     * of virtual functions channels can update internal
     * values as they need to.
     *
     * @param[in] data Data write to register.
     * @param[in] addr Address of register to write.
     */
    virtual void write_reg(uint8_t data, uint16_t addr) override;
};
