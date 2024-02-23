# YAIL (Yet Another Image Loader)

## About ##
Atari 8bit image loader supporting binary PBM and its own YAI format.

If you have a FujiNet you can view streamed images from the search terms you enter.

Using custom display lists YAIL is able to display 220 lines instead of the default 192. This means that when loading a PBM (black and white) image the display will be in Graphics 8 (ANTIC F) at a 320x220 resolution.

## Console ##
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

## Command line ##
Usage: yail [OPTIONS]
  -h this message
  -l <filename> load image file
  -u <url> use this server address
  -s <tokens> search terms

## Server ##
The server is written in python 

To start:
  python3 yailsrv.py

The server uses duckduckgo.com as the source for the images.
It sends your query tokens as the search terms and builds a list of URLs that have the images.
It then downloads the images, converts them to something compatible with gfx9 and then streams to the YAIL.XEX app.
YAIL requests the next image by sending the server a "next" token.
When you quit, the "quit" token is sent.
