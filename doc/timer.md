## Timer

The timer register on the Game Boy seems to cause many people problems. The register is 16 bits long and incremented every 4.096Mhz (or 8.192Mhz double speed), the register can only be read, any writes to the register clear it. Whenever a selected bit in register transitions for 1 to 0 it triggers the operation.

![](timer.png)

Every system clock the DIV register is incremented. At reset this register is set to 0. The DIV register will increment by 4 for every CPU cycle. The upper 8 bits of the DIV register can be read by the CPU at any time. The TAC register is only 3 bits wide (all other bits are ignored and return 1). The lower two bits select which bit of DIV register will control incrementing of the TIMA register. When TAC register bit 2 is set and the selected bit both go to zero the TIMA register will be incremented. So if the selected bit is 1 and the timer is turned off by writing 0 to bit 2 of the TAC, the timer will increment. Whenever the DIV register is written and the selected bit in the DIV register is set the TIMA register will increment.

When the TIMA register is incremented past 0xff there is a hidden bit that is set to indicate overflow. When overflow is set this will trigger and interrupt. Also at the next system clock pulse the TIMA register will be reloaded from the TMA register. If the timer has overflow bit set and the TMA register is written the data will come from TIMA rather then the CPU. If the timer is has overflow set and the TIMA register is updated the TIMA register will be loaded with TMA register rather then CPU data.

Not shown in the diagram is there is a tap off the DIV register from bit 12 (or 13 for double speed) that provides the 512Hz signal that is needed to handle APU frames. The update of the APU frame also occurs when the DIV register transitions from 1 to 0.

## Actual hardware

In the actual hardware the DIV register has a lower 6 bits which are hidden and driven by a 1.024Mhz clock. The DIV register has 8 visible bits above the lower 6 bits, and there are an additional 2 bits which are used to generate 32hz and 16hz. The TIMA register has an additional bit that is set when TIMA overflows and clocked at 1.024Mhz to cause the reload of the TIMA from the TMA register.

## Implemenation

There are several ways to implement the timer. The simpliest which I chose is to use a 16 bit counter that incremented by 4 every Cpu Memory cycle. Access to the DIV register is by looking at the upper 8 bits of this register. You can use an array of values to select which bit to look at based on TAC to decide when to increment TIMA. TIMA should probably be implemented by a register that can hold more then 8 bits and using the top bit to indicate overflow. TMA should be loaded whenever a cycle occurs with TIMA over 255 count.

The other method to use would be two counters, one 6 bit and one 8 bit. The 6 bit counter would be incremented by 1 every CPU memory cycle and when it overflows to 64 the upper DIV register is incremented. This might increase complexity since it will require the 00 setting of TIMA clock bit to be read from the DIV register. Alternatively you could update the upper DIV register whenever the lower 6 bits of the lower DIV register are all 1's, then the upper 2 bits can be used to select the TIMA control bit.

There are other methods of tracking cycles that can be used but these are much more complex. I feel the simplest and most accurate is the 16 bit register incremented by 4 every CPU memory cycle. You could also increment by 1 but this would require shifting the result read from the DIV register by 6 rather then by 8.  