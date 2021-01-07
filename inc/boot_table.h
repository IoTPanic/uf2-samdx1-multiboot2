#ifndef BOOT_TABLE_H
#define BOOT_TABLE_H
#include "uf2.h"

typedef struct {
    uint32_t id;
    uint32_t *flash_start;
    uint32_t flash_len;
    const char *module_name;
    const char *command_line_opts;
} BootVectorEntry;

typedef struct {
    uint32_t crc;
    int module_cnt;
    const BootVectorEntry *entry;
} BootModules;

#endif