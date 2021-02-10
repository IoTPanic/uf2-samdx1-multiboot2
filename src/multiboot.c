#include "multiboot.h"
#include "uf2.h"

uint8_t *find_multiboot_header(uint8_t *start) {
    for(unsigned long i = 0; i<32768; i++) {
        if((uint32_t)start[i] == MULTIBOOT_MAGIC){
            // We found a valid multiboot header!
            return &start[i];
        }
    }
    return NULL;
}

int decode_multiboot_image_headers(uint8_t *start, ImageHeaderTag *result, int result_len) {
    uint32_t h_len = 0;
    // If the magic isn't here, then we are not decoding the right location
    if((uint32_t)start[0] != MULTIBOOT_MAGIC) {
        return -1;
    }
    // Let's ensure we're using the right ISA
    if((uint32_t)start[3] != MULTIBOOT_ARM7M_ISA) {
        return -2;
    }
    // Is the header longer than we support?
    if((uint32_t)start[7] > MB_MAX_IMAGE_HEADER_LEN) {
        return -3;
    }
    // Check the very simple header checksum
    if((uint32_t)start[0] + (uint32_t)start[3] + (uint32_t)start[7] + (uint32_t)start [11] < 1) {
        return -4;
    }

    h_len = start[2];
    int tags = 0;
    uint8_t *cursor = start[4];
    // Lets go through the image
    while(start+h_len > cursor) {
        // "‘type’ is divided into 2 parts. Lower contains an identifier of contents of the rest of the tag. ‘size’ contains the size of tag including header fields. If bit ‘0’ of ‘flags’ (also known as ‘optional’) is set, the bootloader may ignore this tag if it lacks relevant support. Tags are terminated by a tag of type ‘0’ and size ‘8’."
        result[tags].type = (uint16_t)
        ++tags;
    }

 
}