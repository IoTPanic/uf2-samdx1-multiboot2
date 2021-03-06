/* ----------------------------------------------------------------------------
 *         SAM Software Package License
 * ----------------------------------------------------------------------------
 * Copyright (c) 2011-2014, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following condition is met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

/**
 * --------------------
 * SAM-BA Implementation on SAMD21 and SAMD51
 * --------------------
 * Requirements to use SAM-BA :
 *
 * Supported communication interfaces (SAMD21):
 * --------------------
 *
 * SERCOM5 : RX:PB23 TX:PB22
 * Baudrate : 115200 8N1
 *
 * USB : D-:PA24 D+:PA25
 *
 * Pins Usage
 * --------------------
 * The following pins are used by the program :
 * PA25 : input/output
 * PA24 : input/output
 * PB23 : input
 * PB22 : output
 * PA15 : input
 *
 * The application board shall avoid driving the PA25,PA24,PB23,PB22 and PA15
 * signals
 * while the boot program is running (after a POR for example)
 *
 * Clock system
 * --------------------
 * CPU clock source (GCLK_GEN_0) - 8MHz internal oscillator (OSC8M)
 * SERCOM5 core GCLK source (GCLK_ID_SERCOM5_CORE) - GCLK_GEN_0 (i.e., OSC8M)
 * GCLK Generator 1 source (GCLK_GEN_1) - 48MHz DFLL in Clock Recovery mode
 * (DFLL48M)
 * USB GCLK source (GCLK_ID_USB) - GCLK_GEN_1 (i.e., DFLL in CRM mode)
 *
 * Memory Mapping
 * --------------------
 * SAM-BA code will be located at 0x0 and executed before any applicative code.
 *
 * Applications compiled to be executed along with the bootloader will start at
 * 0x2000 (samd21) or 0x4000 (samd51)
 *
 */

#include "uf2.h"
#include "multiboot.h"
#include "boot_table.h"

static void check_start_application(void);

static volatile bool main_b_cdc_enable = false;
extern int8_t led_tick_step;

#if defined(SAMD21)
    #define RESET_CONTROLLER PM
#elif defined(SAMD51)
    #define RESET_CONTROLLER RSTC
#endif

typedef struct {
    bool valid; 
    const BootVectorEntry *entry;
    ImageHeaderTag image_tags[MB_MAX_TAGS];
    uint8_t tags_loaded;
    MemorySpace memory_space;
} BootImage;

typedef struct {
    uint32_t start;
    uint32_t end;
} MemorySpace;

#define KERNEL_OPTS(board, version) -b ## board ## -v ## version

const char kernel_name[] = "ovule";
const char kernel_opts[] = "";

const BootVectorEntry h_boot_entries[] = {
    {
        id: 0,
        flash_start: (uint32_t*)APP_START_ADDRESS,
        flash_len: 16384,
        module_name: kernel_name,
        command_line_opts: kernel_opts,
    }
};

int registered_module_cnt = sizeof(h_boot_entries) / sizeof(h_boot_entries[0]);

/**
 * \brief Check the application startup condition
 *
 */
static void check_start_application(void) {
    uint32_t app_start_address;

    /* Load the Reset Handler address of the application */
    app_start_address = *(uint32_t *)(APP_START_ADDRESS + 4);

    /**
     * Test reset vector of application @APP_START_ADDRESS+4
     * Sanity check on the Reset_Handler address TODO update for MB2 standard
     */
    if (app_start_address < APP_START_ADDRESS || app_start_address > FLASH_SIZE) {
        /* Stay in bootloader */
        return;
    }

#if USE_SINGLE_RESET
    if (SINGLE_RESET()) {
        if (RESET_CONTROLLER->RCAUSE.bit.POR || *DBL_TAP_PTR != DBL_TAP_MAGIC_QUICK_BOOT) {
            // the second tap on reset will go into app
            *DBL_TAP_PTR = DBL_TAP_MAGIC_QUICK_BOOT;
            // this will be cleared after successful USB enumeration
            // this is around 1.5s
            resetHorizon = timerHigh + 50;
            return;
        }
    }
#endif

    if (RESET_CONTROLLER->RCAUSE.bit.POR) {
        *DBL_TAP_PTR = 0;
    }
    else if (*DBL_TAP_PTR == DBL_TAP_MAGIC) {
        *DBL_TAP_PTR = 0;
        return; // stay in bootloader
    }
    else {
        if (*DBL_TAP_PTR != DBL_TAP_MAGIC_QUICK_BOOT) {
            *DBL_TAP_PTR = DBL_TAP_MAGIC;
            delay(500);
        }
        *DBL_TAP_PTR = 0;
    }

    LED_MSC_OFF();

#if defined(BOARD_RGBLED_CLOCK_PIN)
    // This won't work for neopixel, because we're running at 1MHz or thereabouts...
    RGBLED_set_color(COLOR_LEAVE);
#endif






    /* 
    This is the space for the MB2 code, we're going to add a lot of white space to make it obvious
    We need to do a few things first, such as check all the boot modules and read their tags.
    */
    BootImage boot_modules[MB_MAX_MODULES];
    int modules_to_load = 0;

    for(int i = 0; i < registered_module_cnt; i++) {
        // Check for the multiboot magic within the header
        if (* h_boot_entries[i].flash_start != MULTIBOOT_MAGIC) {
            // Not a valid image! We cannot load it
            continue;
        }

        // Check to ensure the module is using the right ISA
        if (*(h_boot_entries[i].flash_start + 4) != MULTIBOOT_ARM7M_ISA){
            continue;
        }

        uint32_t header_len = *(h_boot_entries[i].flash_start + 8);
        // Check the size of the header
        if(header_len < 16 || header_len > MB_MAX_IMAGE_HEADER_LEN) {
            continue;
        }
        
        uint32_t checksum = 0 - (*(h_boot_entries[i].flash_start)) 
            -  *(h_boot_entries[i].flash_start + 4) 
            -  *(h_boot_entries[i].flash_start + 8);

        
        // Compare the calculated checksum with the checksum in the disk image
        if(*(h_boot_entries[i].flash_start + 12)!=checksum) {
            // For now lets still attempt to load it
            // continue;
        }

        boot_modules[modules_to_load].entry = &h_boot_entries[i];

        // Lets create an  array of tags so we can go through them when we create the OS
        // info tags.
        uint32_t *index = h_boot_entries[i].flash_start + 16;
        while((uint32_t)index < ((uint32_t) h_boot_entries[i].flash_start) + header_len) {
            boot_modules[modules_to_load].image_tags[boot_modules[modules_to_load].tags_loaded].type = *(uint16_t*)index;
            boot_modules[modules_to_load].image_tags[boot_modules[modules_to_load].tags_loaded].flags = *(uint16_t*)(index+2);
            boot_modules[modules_to_load].image_tags[boot_modules[modules_to_load].tags_loaded].size = *(index+4);
            boot_modules[modules_to_load].image_tags[boot_modules[modules_to_load].tags_loaded].data = index+8;
            index += boot_modules[modules_to_load].image_tags[boot_modules[modules_to_load].tags_loaded].size + 8;
        }
        ++modules_to_load;
    }

    uint32_t info_requests[MB_MAX_MB_REQ];
    int info_req_cnt = 0;

    // Now we need to validate, and 
    for(int i =0; i < modules_to_load; i++) {
        
        /*
            What do we need for a valid image?

                1. Module memory does not overlap other modules and all modules can fit into memory
                2. Tags are of a valid type and contain valid data

        */

        // Iterator over tags in modules
        for(int m = 0; m < boot_modules[modules_to_load].tags_loaded; m++){
            switch(boot_modules[modules_to_load].image_tags[m].type){
                case MB_IMAGE_HEADER_TYPE_INFO_REQ:
                // Lets assemble out info requests into an array
                for(int iri = 0; iri < boot_modules[modules_to_load].image_tags[m].size / 4; iri++){
                    if(boot_modules[modules_to_load].image_tags[m].data[iri] > 0x21) {
                        info_requests[info_req_cnt++] = boot_modules[modules_to_load].image_tags[m].data[iri];
                    }
                }
                break;
                case MB_IMAGE_HEADER_TYPE_ADDRESS:

                break;
                case MB_IMAGE_HEADER_TYPE_ENTRY_ADDR:
                // TODO must be implemented
                break;
                case MB_IMAGE_HEADER_TYPE_ENTRY_ADDR_EFI_I386:
                case MB_IMAGE_HEADER_TYPE_ENTRY_ADDR_EFI_AMD64:
                case MB_IMAGE_HEADER_TYPE_FLAGS:
                case MB_IMAGE_HEADER_TYPE_FRAMEBUFFER:
                // We are not going to have a display
                continue;
                case MB_IMAGE_HEADER_TYPE_ALIGN_MODULE:
                // TODO we need to align 

                case MB_IMAGE_HEADER_TYPE_EFI_BOOT_SERVICES:
                // TODO we need to assert some error in loading the module
                break;
                case MB_IMAGE_HEADER_TYPE_RELOCATABLE:
            }
        }
    }



























    // Jump to kernel


    /* Rebase the Stack Pointer */
    __set_MSP(*(uint32_t *)APP_START_ADDRESS);

    /* Rebase the vector table base address */
    SCB->VTOR = ((uint32_t)APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk);

    /* Jump to application Reset Handler in the application */
    asm("bx %0" ::"r"(app_start_address));
}

extern char _etext;
extern char _end;

/**
 *  \brief  SAM-BA Main loop.
 *  \return Unused (ANSI-C compatibility).
 */
int main(void) {
    // if VTOR is set, we're not running in bootloader mode; halt
    if (SCB->VTOR)
        while (1) {
        }

#if defined(SAMD21)
    // If fuses have been reset to all ones, the watchdog ALWAYS-ON is
    // set, so we can't turn off the watchdog.  Set the fuse to a
    // reasonable value and reset. This is a mini version of the fuse
    // reset code in selfmain.c.
    if (((uint32_t *)NVMCTRL_AUX0_ADDRESS)[0] == 0xffffffff) {
        // Clear any error flags.
        NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK;
        // Turn off cache and put in manual mode.
        NVMCTRL->CTRLB.reg = NVMCTRL->CTRLB.reg | NVMCTRL_CTRLB_CACHEDIS | NVMCTRL_CTRLB_MANW;
        // Set address to write.
        NVMCTRL->ADDR.reg = NVMCTRL_AUX0_ADDRESS / 2;
        // Erase auxiliary row.
        NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_EAR;
	while (!(NVMCTRL->INTFLAG.bit.READY)) {}
        // Clear page buffer.
        NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC;
	while (!(NVMCTRL->INTFLAG.bit.READY)) {}
        // Reasonable fuse values, including 8k BOOTPROT.
        ((uint32_t *)NVMCTRL_AUX0_ADDRESS)[0] = 0xD8E0C7FA;
        ((uint32_t *)NVMCTRL_AUX0_ADDRESS)[1] = 0xFFFFFC5D;
        // Write the fuses
	NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WAP;
	while (!(NVMCTRL->INTFLAG.bit.READY)) {}
        resetIntoBootloader();
    }

    // Disable the watchdog, in case the application set it.
    WDT->CTRL.reg = 0;
    while(WDT->STATUS.bit.SYNCBUSY) {}

#elif defined(SAMD51)
    // Disable the watchdog, in case the application set it.
    WDT->CTRLA.reg = 0;
    while(WDT->SYNCBUSY.reg) {}

    // Enable 2.7V brownout detection. The default fuse value is 1.7
    // Set brownout detection to ~2.7V. Default from factory is 1.7V,
    // which is too low for proper operation of external SPI flash chips (they are 2.7-3.6V).
    // Also without this higher level, the SAMD51 will write zeros to flash intermittently.
    // Disable while changing level.

    SUPC->BOD33.bit.ENABLE = 0;
    while (!SUPC->STATUS.bit.B33SRDY) {}  // Wait for BOD33 to synchronize.
    SUPC->BOD33.bit.LEVEL = 200;  // 2.7V: 1.5V + LEVEL * 6mV.
    // Don't reset right now.
    SUPC->BOD33.bit.ACTION = SUPC_BOD33_ACTION_NONE_Val;
    SUPC->BOD33.bit.ENABLE = 1; // enable brown-out detection

    // Wait for BOD33 peripheral to be ready.
    while (!SUPC->STATUS.bit.BOD33RDY) {}

    // Wait for voltage to rise above BOD33 value.
    while (SUPC->STATUS.bit.BOD33DET) {}

    // If we are starting from a power-on or a brownout,
    // wait for the voltage to stabilize. Don't do this on an
    // external reset because it interferes with the timing of double-click.
    // "BODVDD" means BOD33.
    if (RSTC->RCAUSE.bit.POR || RSTC->RCAUSE.bit.BODVDD) {
        do {
            // Check again in 100ms.
            delay(100);
        } while (SUPC->STATUS.bit.BOD33DET);
    }

    // Now enable reset if voltage falls below minimum.
    SUPC->BOD33.bit.ENABLE = 0;
    while (!SUPC->STATUS.bit.B33SRDY) {}  // Wait for BOD33 to synchronize.
    SUPC->BOD33.bit.ACTION = SUPC_BOD33_ACTION_RESET_Val;
    SUPC->BOD33.bit.ENABLE = 1;
#endif

#if USB_VID == 0x239a && USB_PID == 0x0013     // Adafruit Metro M0
    // Delay a bit so SWD programmer can have time to attach.
    delay(15);
#endif
    led_init();
    // Set the ESP32 reset pin to a known state in case it was pulled
    // low
    PINOP(ESP_RST, DIRSET);
    PINOP(ESP_RST, OUTSET);

    // Pass a little time to let the ESP32 bootloader start
    for (int i = 1; i < 10000; ++i) {
        asm("nop");
    }

    // Set a white color while we wait for the esp32 to boot
    PININEN(ESP_BUSY);
    RGBLED_set_color(0xA0A0A0);

    // Wait until the ESP32 is connected to the MQTT server
    while(!PINREAD(ESP_BUSY));
    RGBLED_set_color(COLOR_NO_IMAGE);

    logmsg("Start");
    assert((uint32_t)&_etext < APP_START_ADDRESS);
    // bossac writes at 0x20005000
    assert(!USE_MONITOR || (uint32_t)&_end < 0x20005000);

    assert(8 << NVMCTRL->PARAM.bit.PSZ == FLASH_PAGE_SIZE);
    assert(FLASH_PAGE_SIZE * NVMCTRL->PARAM.bit.NVMP == FLASH_SIZE);

    /* Jump in application if condition is satisfied */
    check_start_application();

    /* We have determined we should stay in the monitor. */
    /* System initialization */
    system_init();

    __DMB();
    __enable_irq();

    #if USE_UART
    /* UART is enabled in all cases */
    usart_open();
    #endif

    logmsg("Before main loop");
    // uart_basic_init(SERCOM0, 115200, UART_RX_PAD1_TX_PAD0);
    
    // uart_write_byte(SERCOM0, 's');
    // uart_write_byte(SERCOM0, 't');
    //uart_write_byte(SERCOM0, '\n');

    usb_init();

    // not enumerated yet
    RGBLED_set_color(COLOR_START);
    led_tick_step = 10;
    
    /* Wait for a complete enum on usb or a '#' char on serial line */
    while (1) {
        if (USB_Ok()) {
            if (!main_b_cdc_enable) {
#if USE_SINGLE_RESET
                // this might have been set
                resetHorizon = 0;
#endif
                RGBLED_set_color(COLOR_USB);
                led_tick_step = 1;

#if USE_SCREEN
                screen_init();
                draw_drag();
#endif
            }

            main_b_cdc_enable = true;
        }

#if USE_MONITOR
        // Check if a USB enumeration has succeeded
        // And com port was opened
        if (main_b_cdc_enable) {
            logmsg("entering monitor loop");
            // SAM-BA on USB loop
            while (1) {
                sam_ba_monitor_run();
            }
        }

#else // no monitor
        if (main_b_cdc_enable) {
            process_msc();
        }
#endif

        if (!main_b_cdc_enable) {
            // get more predictable timings before the USB is enumerated
            for (int i = 1; i < 256; ++i) {
                asm("nop");
            }
        }
    }
}
