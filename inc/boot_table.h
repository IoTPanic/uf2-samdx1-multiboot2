#ifndef BOOT_TABLE_H
#define BOOT_TABLE_H
#include "uf2.h"

typedef struct {
    
} BootModules;

typedef struct {
    uint32_t id;
    uint32_t flash_start;
    uint32_t flash_end;
    char *image_name;
    char *command_line_opts;
    BootVectorEntry *next;
} BootVectorEntry;

#endif