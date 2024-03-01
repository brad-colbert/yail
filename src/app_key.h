#ifndef APP_KEY_H
#define APP_KEY_H

#include "fujinet-io.h"

unsigned char sio_openkey(AppKeyDataBlock* data, unsigned char open_mode, unsigned char key);

void get_settings(void);

void put_settings(void);

#endif