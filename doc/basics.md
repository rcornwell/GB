## Basics of Emulation

Emulation of a system consists of finding the central component of the system and stepping it through its steps. For most computer systems this is the CPU. The CPU basically operates in a simple loop. Fetch an instruction from memory, decode the instruction to determine what if any operands it might need to execute the instruction then executing the instruction. For the emulator writer this consists of fetching from memory the instruction decoding it and executing it and updating the system state to match what the instruction would have changed. At times the CPU might modify the state of devices, depending on the device the external (to CPU) change might be instantaneous or it might take many cycles to occur. For example the CPU might request a device to read something of disk or other external storage, this could take a very long time if the operation requires seeking a head and waiting for the disk to move into place. In a game console the CPU might have to wait until a scan line is being displayed to update some color palette register to make the display look correct.

Depending on the system being emulated the external delay might not be significant or it could mean the difference between the system working and not working. In terms of older game consoles it is important to get the timing of the screen and other events (such as timers etc), to be as close as possible to the original. Many games rely on things occurring at the exact time either to update game logic, or make a better display than the basic hardware would allow. These delays can either be tracked by use of an event system. Where the amount of time until the action is complete is computed in terms of cycles and when time expires an event is generated. The other option is to just track cycles.

For the Game Boy since everything is controlled off the same clock it is easier to track CPU memory accesses and update external state as needed. This can either be done in the CPU functions (or object) or in the Memory system (or object). There are a couple of ways of doing this:

1) At each cycle the external objects are updated if needed.
2) After the CPU executes and instruction the external objects are updated to reflect as if so many cycles have passed.
3) An event system can be told how many cycles have occurred and compute if any events need to be fired.

For this emulator I choose to allow the Memory object to handle cycle update. So as the CPU object requests data from memory the external objects are advanced on time step as needed. For some of the diagnostic tests available the exact cycle the update occurs is important. I am not sure if any games require this exact level of detail. But it is easier to implement this way then by attempting to compute this ahead of time when it might occur. For efficient emulations it is important to keep logic as simple and efficient as possible. The less condition checking per cycle the best.