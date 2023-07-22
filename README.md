# Game Boy Emulator

This is a sample C++ emulator for a Game Boy game console. It is written to be simple yet cycle accurate. The Emulator does not provide many features. It was written as a speed code project to see how quickly a emulator could be put together. While writing this emulator I found a lot of conflicts in the available documentation. As time permits I will attempt to document how some of the questionable areas of the devices function.

The heart of the emulator is the CPU. However all timing goes through the Memory object. This is because the rest of the emulator needs to be synced to the CPU.

Currently the emulator has only been tested on Linux. It requires SDL2-dev and SDL2-mixer-dev. For running tests you will need CppUTest package to be installed. If you wish to generate documentation on the emulator you will need Doxygen installed. Also either at -DBUILD_DOC=ON or change the option BUILD_DOC ... OFF, to BUILD_DOC... ON in the top level CMakeFile.txt.

To build the emulator either git clone it or extract the zip file from github. In the emulator directory do:

> mkdir build  
> cd build  
> cmake ..  
> make  

The emulator accepts a couple arguments:

- -# Where # is the scale factor for image display.
- -t Will provide debugging trace of the execution of instructions.
- -p # Specifies the port to open to connect the serial link to.
- -h *hostname* Specifies the host to connect to.
- Cartridge.gb to load.
- Optional .sav file.

If no save file is given the emulator will create one with the same path as the cartridge if the cartridge indicates that the original cartridge had a battery. If the file name is not given the file extension of the cartridge will be replaced with "sav". If a file name is given it will be loaded regardless of whether the cartridge indicates it has a battery. However it will not be saved on exit from the emulator.

When the emulator is running it will accept the following key presses. "Q" will exit the emulator. The arrow keys perform the function of the game boy arrow keys. "X" is the A Button. "Z" is the B button. Left shift is the select button. And "enter" is the start button.

At the moment serial link just outputs to the standard error, when completed specifying a host will form a connection with an emulator running at give port number. If no host is specified the emulator will listen for connections on the port specified.



