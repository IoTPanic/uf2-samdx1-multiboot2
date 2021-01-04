#ifndef MULTIBOOT2
#include "uf2.h"

#define MULTIBOOT2

#define MULTIBOOT_MAGIC 0xE85250D6
#define MULTIBOOT_ARM7M_ISA 0x0A70
#define MULTIBOOT_CHECKSUM_SUM 0x0

#define MB_IMAGE_HEADER_TYPE_INFO_REQ = 0x01
#define MB_IMAGE_HEADER_TYPE_ADDRESS = 0x02
#define MB_IMAGE_HEADER_TYPE_ENTRY_ADDR = 0x03
#define MB_IMAGE_HEADER_TYPE_ENTRY_ADDR_EFI_I386 = 0x08
#define MB_IMAGE_HEADER_TYPE_ENTRY_ADDR_EFI_AMD64 = 0x09
#define MB_IMAGE_HEADER_TYPE_FLAGS = 0x04
#define MB_IMAGE_HEADER_TYPE_FRAMEBUFFER = 0x05
#define MB_IMAGE_HEADER_TYPE_ALIGN_MODULE = 0x06
#define MB_IMAGE_HEADER_TYPE_EFI_BOOT_SERVICES = 0x07
#define MB_IMAGE_HEADER_TYPE_RELOCATABLE = 0x10 // Weird its not 0xA but thats the spec

#define MB_BOOT_INFO_BASIC_MEM_INFO 0x04
#define MB_BOOT_INFO_BIOS_DEVICE 0x05
#define MB_BOOT_INFO_BOOT_CMD_LINE 0x01
#define MB_BOOT_INFO_MODULES 0x03
#define MB_BOOT_INFO_ELF_SYMBOLS 0x09
#define MB_BOOT_INFO_MEM_MAP 0x06
#define MB_BOOT_INFO_BOOTL_NAME 0x02
#define MB_BOOT_INFO_APM_TABLE 0x10
#define MB_BOOT_INFO_VBE_INFO 0x07
#define MB_BOOT_INFO_FRAMEBUFFER_INFO 0x08
#define MB_BOOT_INFO_EFI32_TABLE 0x11
#define MB_BOOT_INFO_EFI64_TABLE 0x12
#define MB_BOOT_INFO_SMBIOS_TABLE 0x13
#define MB_BOOT_INFO_ACPI_OLD_RSDP 0x14
#define MB_BOOT_INFO_ACPI_NEW_RSDP 0x15
#define MB_BOOT_INFO_NETWORKING 0x16
#define MB_BOOT_INFO_EFI_MEM_MAP 0x17
#define MB_BOOT_INFO_EFI_BOOT_SERV_NTERM 0x18
#define MB_BOOT_INFO_EFI32_IMAGE_HPTR 0x19
#define MB_BOOT_INFO_EFI64_IMAGE_HPTR 0x20
#define MB_BOOT_INFO_LOAD_ADDR 0x21

// Name from Makefile
const char bootloader_name[] = NAME;

// The image header is prependded to a image we wish to boot which will inform the
// bootloader of basic details
typedef struct {
    uint32_t type;
    uint16_t flags;
    uint32_t size;
    uint32_t *tags;
    ImageHeaderTag *next;
} ImageHeaderTag;

// These tags are given to the OS after boot in order to be aware of other loaded
// modules, where in physical memory itself was loaded, etc.
typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t *data;
    BootInfoTag *next;
} BootInfoTag;

#endif