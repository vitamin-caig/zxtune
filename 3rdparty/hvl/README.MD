About HivelyTracker, from IRIS & Up Rough!

Hively Tracker is a tracker program based upon the AHX format created in the mid '90s by Dexter and Pink of Abyss. The format was relatively popular, and many songs were created and used in scene productions and games. AHX was designed to create a very SID-like sound on the Amiga.

HivelyTracker can import and export modules and instruments in the AHX format, but it also improves on AHX in several ways and therefore has its own instrument and module formats.

HivelyTracker offers the following features over AHX:

- Multichannel (4 to 16 channels)
- Per-channel stereo panning
- Two commands per note instead of one
- Ring modulation
- A more feature rich editor


HivelyTracker was coded by Xeron/IRIS
GUI design and skins by Spot/Up Rough

Compiling
=========

The makefile at the root of the repository is for Amiga OS only and will not work for other platforms.

Building for Amiga OS
---------------------

    make

Building for Linux
------------------

    cd sdl
    make -f Makefile.linux
    
Building for Haiku
------------------

    cd sdl
    make -f Makefile.haiku
    
Building for Windows
--------------------

    cd sdl
    make -f Makefile
    
Building for MacOS
------------------

An XCode project is provided in the SDL directory.
