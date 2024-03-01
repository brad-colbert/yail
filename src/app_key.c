#include "settings.h"
#include "app_key.h"
#include "fujinet-io.h"

#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <atari.h>

#define FN_CREATOR_ID 0xD00D
#define FN_APP_ID 0x01
#define FN_URL_KEY_ID 0x01

#define DEFAULT_URL "N:TCP://fujinet.online:5556/"
extern Settings settings;

unsigned char sio_openkey(AppKeyDataBlock* data, unsigned char open_mode, unsigned char key)
{
    data->open.creator = FN_CREATOR_ID;
    data->open.app = FN_APP_ID;
    data->open.key = key;
    data->open.mode = open_mode;
    data->open.reserved = 0x00;

    return fn_io_appkey_open(&data->open);
}

void get_settings()
{
    AppKeyDataBlock data;
    uint8_t r;

    sio_openkey(&data, 0, FN_URL_KEY_ID);

    r = fn_io_appkey_open(&data.open);

    if (1 == r) // key doesn't exist. write the default.
    {
        r = sio_openkey(&data, 1, FN_URL_KEY_ID);

        if(1 == r)
            return;

        strncpy((char *)data.write.value, DEFAULT_URL, MAX_APPKEY_LEN);
        r = fn_io_appkey_write(sizeof(DEFAULT_URL), &data.write);

        if(1 == r)
            return;

        strncpy(settings.url, DEFAULT_URL, MAX_APPKEY_LEN);
    }

    r = fn_io_appkey_read(&data.read);

    if (1 == r)
        return;

    strncpy(settings.url, (char*)data.read.value, MAX_APPKEY_LEN);

    // Add more settings below...
}

void put_settings()
{
    AppKeyDataBlock data;
    uint8_t r;

    r = sio_openkey(&data, 1, FN_URL_KEY_ID);

    if (1 == r) // unable to open or create the key for write
        return;

    strncpy((char *)data.write.value, settings.url, MAX_APPKEY_LEN);
    r = fn_io_appkey_write(sizeof(DEFAULT_URL), &data.write);

    if (1 == r) // problem writting the key
        return;
}