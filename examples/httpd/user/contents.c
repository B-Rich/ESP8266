// ----------------------------------------------------------------------------
// Routines for Saving and retreiving Contents data.
// The flash RAM is used to store the contents in a binary array. Data is
// received and sent as JSON.
// ----------------------------------------------------------------------------
#include <ets_sys.h>
#include <string.h>
#include <osapi.h>
#include <spi_flash.h>
#include <json/jsonparse.h>
#include "espmissingincludes.h"
#include "stdout.h"
#include "contents.h"

// ----------------------------------------------------------------------------
// Global definitions and variables
// ----------------------------------------------------------------------------
#define FLASHCONTENTS     0x11000               // Flash RAM location for user memory
#define FLASHSECTOR       FLASHCONTENTS/0x1000  // Flash RAM location for user memory
#define RECORD_LEN        32                    // each record is 32 bytes

// ----------------------------------------------------------------------------
// Get it back from Flash
// ----------------------------------------------------------------------------
static void getfromFlash(uint32 flash_addr, char *str, int len)
{
    SpiFlashOpResult ret;
    ret = spi_flash_read(flash_addr, (uint32 *)str, len);
    if(ret != SPI_FLASH_RESULT_OK) {
        os_printf("flash read error: %d\r\n", (int)ret);
    }
}

// ----------------------------------------------------------------------------
// CGI that reads the json db and returns it
// ----------------------------------------------------------------------------
int ICACHE_FLASH_ATTR get_contents(char *buffer)
{
    char str[32];                       // place for just the content item
    uint32 flash_addr = FLASHCONTENTS;  //  user memory

    os_sprintf(buffer,"{\"contents\":[\n");
    int n = strlen(buffer);
    for(int i = 0; i < 32; i++) {
        getfromFlash(flash_addr, str, 32);   // ingredient str len is 32 fixed
        if(str[0] == 0xFF) {                 // This means it is an empty slot
            n += os_sprintf(&buffer[n],"{\"name\":\"empty\",\"amount\":0},");
        }
        else {
            int val = (str[30] << 8) | str[31];
            str[30] = 0; // precautionary
            n += os_sprintf(&buffer[n],"{\"name\":\"%s\",\"amount\":%d},", str, val);
        }
        flash_addr += 32;
    }
    n--;
    n += os_sprintf(&buffer[n],"]}\n");
    return(n);
}

// ----------------------------------------------------------------------------
// Erase the sector from flash so it is ready to be reprogrammed
// ----------------------------------------------------------------------------
static void eraseFlashsector(int sector)
{
    SpiFlashOpResult ret;
    ret = spi_flash_erase_sector(sector);
    if(ret != SPI_FLASH_RESULT_OK) {
        os_printf("flash sector erase error: %d\r\n", (int)ret);
    }
}

// ----------------------------------------------------------------------------
// Save contentst to Flash
// ----------------------------------------------------------------------------
static void savetoFlash(char *buffer, int len)
{
    SpiFlashOpResult ret;
    uint32 flash_addr = FLASHCONTENTS;  //  user memory

    ret = spi_flash_write(flash_addr, (uint32 *)buffer, len);
    if(ret != SPI_FLASH_RESULT_OK) os_printf("error ret: %d\r\n", (int)ret);
}

// ----------------------------------------------------------------------------
// CGI that receives the json list of canister contents
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR put_contents(char *jsdata,  int len)
{
    char type;
    char str[80];
    char buffer[1024];    // contents names

    struct jsonparse_state state;
    struct jsonparse_state *js = &state;
    jsonparse_setup(js, jsdata, len);
    type = jsonparse_next(js);
    if(type != JSON_TYPE_OBJECT) {
        os_printf("Not a valid JSON file(1)\n");
        return;
    }
    type = jsonparse_next(js);
    if(type != JSON_TYPE_PAIR_NAME) {
        os_printf("Not a valid JSON file(2)\n");
        return;
    }
    jsonparse_copy_value(js, str, 80);
    if(jsonparse_strcmp_value(js, "contents") != 0) {
        os_printf("Not a valid Contents file\n");
        return;
    }
    int n = 0;
    for(int i=0; i < 1024; i++) buffer[i] = 0;
    while(js->pos < js->len) {
        type = jsonparse_next(js);
        if(type != JSON_TYPE_PAIR_NAME) continue;
        if(jsonparse_strcmp_value(js, "name") == 0) {
            type = jsonparse_next(js);
            if(type == JSON_TYPE_PAIR) {
                type = jsonparse_next(js);
                if(type == JSON_TYPE_STRING) {
                    jsonparse_copy_value(js, &buffer[n], 30);
                    n += 32;
                }
            }
        }
        if(jsonparse_strcmp_value(js, "amount") == 0) {
            type = jsonparse_next(js);
            if(type == JSON_TYPE_PAIR) {
                type = jsonparse_next(js);
                int val = 0;
                if(type == JSON_TYPE_NUMBER) val = jsonparse_get_value_as_int(js);
                else if(type == JSON_TYPE_STRING) {
                    jsonparse_copy_value(js, str, 16);
                    val = atoi(str);
                }
                buffer[n-2] = (uint8_t)(val >> 8);
                buffer[n-1] = (uint8_t)(val & 0xff);
            }
        }
        if(n > 1024) {
            os_printf("file is too large to save all\n");
            break;
        }
    }
    os_printf("json object parsed\n");
    // now that we parsed all that JSON bullshit, now we can finnaly save
    // our data into flash, but first, we have to erase a sector
    eraseFlashsector(FLASHSECTOR);
    savetoFlash(buffer, n);
    os_printf("Contents data saved to flash\n");
}

// ----------------------------------------------------------------------------

