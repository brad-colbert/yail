# imgload
Atari 8bit image loader supporting binary PBM and PGM formats.

Using custom display lists IMGLOAD is able to display 220 lines instead of the default 192. This means that when loading a PBM (black and white) image the display will be in Graphics 8 (ANTIC F) at a 320x220 resolution.

IMGLOAD has a simple, single line, command interface.  To activate it simply press a key and the command line will appear at the bottom of the screen.
Commands:

  - help              - List of commands
  - load <filename>   - Loads the specified PBM/PGM files and now a new YAI file.
  - save <filename>   - Saves the current image and graphics state to a YAI file.
  - cls               - Clears the screen
  - gfx #  (0, 8, 9)  - Change the graphics mode to the number specified
  - quit              - Quit the application

Tested on and works with the Atari 800.  Other models, **YMMV**
