#ifndef SETTINGS_H
#define SETTINGS_H

//
#define SERVER_URL_SIZE 64

// A structure for holding the application runtime settings
typedef struct {
    char url[SERVER_URL_SIZE];
} Settings;

#endif