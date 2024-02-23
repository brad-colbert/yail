# YAIL (Yet Another Image Loader)
Atari 8bit image loader supporting binary PBM and its own YAI format.

If you have a FujiNet you can view streamed images from the search terms you enter.

Using custom display lists YAIL is able to display 220 lines instead of the default 192. This means that when loading a PBM (black and white) image the display will be in Graphics 8 (ANTIC F) at a 320x220 resolution.

YAIL has a simple text console for interaction that is activated when you start to type.
Commands:

  - help                  - List of commands
  - load <filename>       - Loads the specified PBM/PGM files and now a new YAI file.
  - save <filename>       - Saves the current image and graphics state to a YAI file.
  - cls                   - Clears the screen
  - gfx #  (0, 8, 9)      - Change the graphics mode to the number specified
  - stream <search terms> - Stream images (gfx 9) from the yailsrv.py.
  - set server <url>      - Give the N:TCP URL for the location of the yailsrv.py.
                            Ex: set server N:TCP://192.168.1.205:9999/
  - quit              - Quit the application

Tested on and works with the Atari 800XL.  Other models, **YMMV**
