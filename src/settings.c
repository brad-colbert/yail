#include "settings.h"
#include "graphics.h"
#include "utility.h"
#include "fujinet-fuji.h"

#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <atari.h>

#define FN_CREATOR_ID 0xD00D
#define FN_APP_ID 0x01
#define FN_URL_KEY_ID 0x01
#define FN_GFX_KEY_ID 0x02

#define DEFAULT_URL "N:TCP://fujinet.online:5556/"
#define DEFAULT_GFX_MODE GRAPHICS_8

// Globals
Settings settings;

// Externals
extern byte buff[];

/*
unsigned char sio_openkey(AppKeyDataBlock* data, unsigned char open_mode, unsigned char key)
{
    data->open.creator = FN_CREATOR_ID;
    data->open.app = FN_APP_ID;
    data->open.key = key;
    data->open.mode = open_mode;
    data->open.reserved = 0x00;

    return fn_io_appkey_open(&data->open);
}
*/

uint8_t get_settings()
{
    #if 0
    AppKeyDataBlock data;
    uint8_t r;

    // Get/Create the URL setting
    memset(&data, 0, sizeof(AppKeyDataBlock));

    r = sio_openkey(&data, 0, FN_URL_KEY_ID);
    if (0 == r)
    {
        r = fn_io_appkey_read(&data.read);  // Try to read the key

        if (1 == r) // key doesn't exist. write the default.
        {
            byte keylen = strlen(DEFAULT_URL);

            strncpy(settings.url, DEFAULT_URL, MAX_APPKEY_LEN);
            sio_openkey(&data, 1, FN_URL_KEY_ID);
            strncpy((char *)data.write.value, settings.url, MAX_APPKEY_LEN);
            r = fn_io_appkey_write(keylen, &data.write);

            if(1 == r)
                return r;
        }
        else
        {
            memset(settings.url, 0, SERVER_URL_SIZE);
            strncpy(settings.url, (char*)data.read.value, data.read.length);
        }
    }
    else // use default
        strncpy(settings.url, DEFAULT_URL, MAX_APPKEY_LEN);

    // Get/Create the gfx mode setting
    memset(&data, 0, sizeof(AppKeyDataBlock));

    r = sio_openkey(&data, 0, FN_GFX_KEY_ID);
    if (0 == r)
    {
        r = fn_io_appkey_read(&data.read);    // Try to read the key

        if (1 == r) // key doesn't exist. write the default.
        {
            sio_openkey(&data, 1, FN_GFX_KEY_ID);
            data.write.value[0] = DEFAULT_GFX_MODE;
            r = fn_io_appkey_write(1, &data.write);

            if(1 == r)
                return r;

            setGraphicsMode(DEFAULT_GFX_MODE);   
        }
        else
            setGraphicsMode(((byte*)data.read.value)[0]); // for a test  | GRAPHICS_BUFFER_TWO);
    }
    else // use default
        setGraphicsMode(DEFAULT_GFX_MODE);
    #endif

    uint16_t count;

    // Set the base key information
    fuji_set_appkey_details(FN_CREATOR_ID, (uint8_t)FN_APP_ID, MAX_APPKEY_LEN);

    // URL Read the key
    if (0 == fuji_read_appkey(FN_URL_KEY_ID, &count, (uint8_t *)buff))
    {
        uint16_t keylen = strlen(DEFAULT_URL);

        // Key doesn't exist. Write the default URL
        strncpy(settings.url, DEFAULT_URL, SERVER_URL_SIZE);
        fuji_write_appkey(FN_URL_KEY_ID, keylen, (uint8_t *)&settings.url);
    }
    else // key exists
    {
        // Already in settings.url
        strncpy(settings.url, (char*)buff, count);
        settings.url[count] = 0x00;
    }

    // GFX Read the key
    if (0 == fuji_read_appkey(FN_GFX_KEY_ID, &count, (uint8_t *)buff))
    {
        // Key doesn't exist. Write the default GFX mode
        buff[0] = DEFAULT_GFX_MODE;
        fuji_write_appkey(FN_GFX_KEY_ID, 1, (uint8_t *)&buff[0]);
    }
    setGraphicsMode((byte)buff[0]);

    // Add more settings below...

    return 0;
}

uint8_t put_settings(byte select)
{
    #if 0
    AppKeyDataBlock data;
    uint8_t r;

    switch(select)
    {
        case SETTINGS_NONE:
            return 0;
        case SETTINGS_URL:
            {
                byte keylen = strlen(settings.url);

                r = sio_openkey(&data, 1, FN_URL_KEY_ID);

                if (1 == r)
                    return 1;
                    
                strncpy((char *)data.write.value, settings.url, MAX_APPKEY_LEN);
                r = fn_io_appkey_write(keylen, &data.write);

                if(1 == r)
                    return r;
            }
            break;
        case SETTINGS_GFX:
            {
                r = sio_openkey(&data, 1, FN_GFX_KEY_ID);

                if (1 == r)
                    return 1;

                data.write.value[0] = settings.gfx_mode & ~GRAPHICS_CONSOLE_EN;  // Don't capture the console bit
                r = fn_io_appkey_write(1, &data.write);

                if(1 == r)
                    return r;
            }
            break;
        default:
            return 1;
    }
    #endif
    switch(select)
    {
        case SETTINGS_NONE:
            return 0;
        case SETTINGS_URL:
        {
            uint16_t urllen = strlen(settings.url);

            // Configure the key for writing
            fuji_set_appkey_details(FN_CREATOR_ID, (uint8_t)FN_APP_ID, MAX_APPKEY_LEN);
            return fuji_write_appkey(FN_URL_KEY_ID, urllen, (uint8_t *)settings.url);
        }
        break;
        case SETTINGS_GFX:
        {
            fuji_set_appkey_details(FN_CREATOR_ID, (uint8_t)FN_APP_ID, MAX_APPKEY_LEN);
            return fuji_write_appkey(FN_GFX_KEY_ID, 1, (uint8_t *)&settings.gfx_mode);
        }
        break;
        default:
            return 0;
    }
}