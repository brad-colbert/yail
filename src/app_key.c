#include "graphics.h"
#include "settings.h"
#include "app_key.h"
#include "utility.h"
#include "fujinet-io.h"

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

//
extern Settings settings;
extern byte buff[];

unsigned char sio_openkey(AppKeyDataBlock* data, unsigned char open_mode, unsigned char key)
{
    data->open.creator = FN_CREATOR_ID;
    data->open.app = FN_APP_ID;
    data->open.key = key;
    data->open.mode = open_mode;
    data->open.reserved = 0x00;

    return fn_io_appkey_open(&data->open);
}

uint8_t get_settings()
{
    AppKeyDataBlock data;
    uint8_t r;

    // Get/Create the URL setting
    memset(&data, 0, sizeof(AppKeyDataBlock));

    sio_openkey(&data, 0, FN_URL_KEY_ID);
    r = fn_io_appkey_read(&data.read);  // Try to read the key

    if (1 == r) // key doesn't exist. write the default.
    {
        byte keylen = strlen(DEFAULT_URL);
        //cputs("Creating default URL key\n\r");
        strncpy(settings.url, DEFAULT_URL, MAX_APPKEY_LEN);
        sio_openkey(&data, 1, FN_URL_KEY_ID);
        strncpy((char *)data.write.value, settings.url, MAX_APPKEY_LEN);
        //cprintf("Writing %s to key %d\n\r", (char*)data.write.value, FN_URL_KEY_ID);
        r = fn_io_appkey_write(keylen, &data.write);

        if(1 == r)
        {
            //cputs("Problem writing URL key\n\r");
            return r;
        }
    }
    else
    {
        //cprintf("Read URL key %d:%s\n\r", data.read.length, (char*)data.read.value);
        memset(settings.url, 0, SERVER_URL_SIZE);
        strncpy(settings.url, (char*)data.read.value, data.read.length);
    }

    // Get/Create the gfx mode setting
    memset(&data, 0, sizeof(AppKeyDataBlock));

    sio_openkey(&data, 0, FN_GFX_KEY_ID);
    r = fn_io_appkey_read(&data.read);    // Try to read the key

    if (1 == r) // key doesn't exist. write the default.
    {
        //cputs("Creating default GFX key\n\r");
        //settings.gfx_mode = DEFAULT_GFX_MODE;
        sio_openkey(&data, 1, FN_GFX_KEY_ID);
        data.write.value[0] = DEFAULT_GFX_MODE;
        //cprintf("Writing %02X to key %d\n\r", data.write.value[0], FN_GFX_KEY_ID);
        r = fn_io_appkey_write(1, &data.write);

        if(1 == r)
        {
            //cputs("Problem writing GFX key\n\r");
            return r;
        }

        setGraphicsMode(DEFAULT_GFX_MODE);   
    }
    /*
    else
    {
        cprintf("Read GFX key %d:%02x\n\r", data.read.length, ((byte*)data.read.value)[0]);
        settings.gfx_mode = ((byte*)data.read.value)[0];
    }
    */
    //cprintf("Read GFX key %d:%02x\n\r", data.read.length, ((byte*)data.read.value)[0]);
    //cgetc();
    else
        setGraphicsMode(((byte*)data.read.value)[0]);   

    // Add more settings below...

    return 0;
}

uint8_t put_settings()
{
    AppKeyDataBlock data;
    uint8_t r;

    // Get the URL settings
    r = sio_openkey(&data, 1, FN_URL_KEY_ID);

    if (1 == r) // unable to open or create the key for write
        return r;

    strncpy((char *)data.write.value, settings.url, MAX_APPKEY_LEN);
    r = fn_io_appkey_write(sizeof(DEFAULT_URL), &data.write);

    if (1 == r) // problem writting the key
        return r;

    buff[0] = (char)settings.gfx_mode;
    buff[1] = 0x0;
    
    //r = write_value_to_key(FN_GFX_KEY_ID, &data, buff);
    
    // Get the GFX mode settings
    r = sio_openkey(&data, 1, FN_GFX_KEY_ID);

    if (1 == r) // unable to open or create the key for write
        return r;

    strncpy((char *)data.write.value, buff, 2);
    r = fn_io_appkey_write(sizeof(DEFAULT_URL), &data.write);

    if (1 == r) // problem writting the key
    {
        cputs("Problem writing GFX key\n\r");
        return r;
    }

    return r;
}