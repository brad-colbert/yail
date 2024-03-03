#ifndef SETTINGS_H
#define SETTINGS_H

#include "types.h"
#include "fujinet-io.h"

//
#define SERVER_URL_SIZE MAX_APPKEY_LEN
#define SETTINGS_NONE 0
#define SETTINGS_URL  1
#define SETTINGS_GFX  2

// A structure for holding the application runtime settings
typedef struct {
    char url[SERVER_URL_SIZE];
    byte gfx_mode;
} Settings;

//unsigned char sio_openkey(AppKeyDataBlock* data, unsigned char open_mode, unsigned char key);

uint8_t get_settings(void);

uint8_t put_settings(byte select);

#endif