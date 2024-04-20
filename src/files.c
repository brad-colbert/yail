#ifdef YAIL_BUILD_FILE_LOADER
// Copyright (C) 2021 Brad Colbert
#include "files.h"
#include "console.h"
#include "readNetPBM.h"
#include "graphics.h"
#include "settings.h"
#include "version.h"
#include "utility.h"

#include <conio.h>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h> 

// externals
extern ImageData image;
extern byte buff[];
//extern byte CURRENT_MODE;
extern void graphics_9_console_dl;
extern Settings settings;

// returns a token based on the filetype determined from the extension
byte imageFileType(const char filename[])
{
    byte len = strlen(filename);

    if(len > 4)  // ext plus .
    {
        char* ext = 0x0;
        int i;

        // try to get the extension
        for(i = 0; i < len; ++i)
        {
            if((filename[i] == 0x0E) || (filename[i] == 0x2E))
            {
                if((len - (i+1)) > 2)
                {
                    ext = (char*)&filename[i + 1];
                    break;
                }
            }
        }

        if(ext)
        {
            if(toupper(ext[0]) == 'P')
            {
                if(toupper(ext[1]) == 'B')
                    return FILETYPE_PBM;
                if(toupper(ext[1]) == 'G')
                    return FILETYPE_PGM;                
            }
            else if(toupper(ext[0]) == 'Y')
                if(toupper(ext[1]) == 'A')
                    return FILETYPE_YAI;
        }
    }
}

void saveFile(const char filename[])
{
    size_t size;
    int fd = open(filename, O_WRONLY);

    if(fd >= 0)
    {
        byte b;

        // Write the version
        b = MAJOR_VERSION;
        write(fd, &b, 1);
        b = MINOR_VERSION;
        write(fd, &b, 1);
        b = BUILD_VERSION;
        write(fd, &b, 1);

        // Write the graphics mode
        b = settings.gfx_mode & ~GRAPHICS_CONSOLE_EN;
        write(fd, &b, 1);

        // Write the DLs
        #define IMAGE_DL_SIZE 199
        b = DL_TOKEN;
        write(fd, &b, 1);
        write(fd, &OS.sdlst, sizeof(unsigned));
        write(fd, OS.sdlst, 199);

        #define IMAGE_BLOCK_ONE_TWO_SIZE 4080
        #define IMAGE_BLOCK_THREE_SIZE 640
        b = MEM_TOKEN;
        size = IMAGE_BLOCK_ONE_TWO_SIZE;
        write(fd, &b, 1);
        write(fd, &size, sizeof(size_t));
        write(fd, image.data, IMAGE_BLOCK_ONE_TWO_SIZE);

        write(fd, &b, 1);
        write(fd, &size, sizeof(size_t));
        write(fd, (image.data+0x1000), IMAGE_BLOCK_ONE_TWO_SIZE);

        size = IMAGE_BLOCK_THREE_SIZE;
        write(fd, &b, 1);
        write(fd, &size, sizeof(size_t));
        write(fd, (image.data+0x2000), IMAGE_BLOCK_THREE_SIZE);

        //
        close(fd);
    }
}

// load a file into the graphics frame buffer
byte load_image_file(const char filename[])
{
    int fd = open(filename, O_RDONLY);
    if(fd >= 0)
    {
        byte file_type = imageFileType(filename);

        //cprintf("Loading... %s", filename);

        switch(file_type)
        {
            case FILETYPE_YAI:
            {
                const ushort BYTES_PER_LINE = 40;
                const ushort LINES = 220;
                const ushort BUFFER_BLOCK_SIZE = 4080;
                byte* buffer = image.data;
                ushort buffer_block_remaining = BUFFER_BLOCK_SIZE;
                FileHeader header;
                byte block_type;
                unsigned block_size = 0;
                size_t ttl_bytes = 0;
                size_t n = 0;

                // Read the header
                n = read(fd, (void*)&header, sizeof(header));
                ttl_bytes += n;

                //
                setGraphicsMode(header.gfx);

                // Read the next block of info.  We are assuming is a display list.
                while(n > 0)
                {
                    n = read(fd, (void*)&block_type, sizeof(block_type));
                    ttl_bytes += n;

                    switch(block_type)
                    {
                        case DL_TOKEN:
                        {
                            n = read(fd, (void*)&block_size, sizeof(block_size));
                            ttl_bytes += n;
                            n = read(fd, (void*)buff, block_size);
                            ttl_bytes += n;
                        }
                        break;

                        case MEM_TOKEN:
                        {
                            // Read how big this block is.  If the block is larger than the next memory block
                            // we need to adjust the size that we read.  We then need to read the remaining
                            // into the next memory block.  Going to cheat a little and assume that none of the
                            // blocks have more than 4K of data.
                            //unsigned next_mem_block = buffer + image_block_size;
                            n = read(fd, (void*)&block_size, sizeof(block_size));
                            ttl_bytes += n;

                            // Check if the data we have available is greater than the current buffer block
                            // size.  If so, we will read only enough data to fill the buffer block and then
                            // read the remaining into the next buffer block.
                            if(block_size > buffer_block_remaining)
                            {
                                // Read enough to fill the buffer block
                                n += read(fd, (void*)buffer, buffer_block_remaining);
                                
                                // Move the buffer pointer to the next block
                                buffer += buffer_block_remaining + 16;  // 16 means that the last line will not cross a 4K boundary

                                // Read the remaining data into the next buffer block
                                n += read(fd, (void*)buffer, block_size - buffer_block_remaining);

                                // Reset the buffer block remaining
                                buffer += (block_size - buffer_block_remaining);
                                buffer_block_remaining = BUFFER_BLOCK_SIZE - (block_size - buffer_block_remaining);
                            }
                            else
                            {
                                n += read(fd, (void*)buffer, block_size);
                                buffer += block_size;
                                buffer_block_remaining -= block_size;
                            }

                            ttl_bytes += n;
                        }
                        break;
                        default:
                            n = 0;
                    }
                }

                return n;
            }
            break;
            
            case FILETYPE_PBM:
            case FILETYPE_PGM:
                switch(file_type)
                {
                    case FILETYPE_PBM:
                        hide_console();
                        setGraphicsMode(GRAPHICS_8);
                        readPBM(fd);
                        break;
                    case FILETYPE_PGM:
                        hide_console();
                        setGraphicsMode(GRAPHICS_9);
                        readPGM(fd);    // on hold for now
                        readPBM(fd);
                        break;
                };
            break; 
        }

        close(fd);
    }
    else
    {
        cprintf("ERROR: Problem opening file %d:*%s*", strlen(filename), filename);
        cgetc();
    }

    return 0;
}
#endif