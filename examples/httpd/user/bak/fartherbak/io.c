// ----------------------------------------------------------------------------
// Basic IO Routines
// ----------------------------------------------------------------------------
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "gpio.h"
#include "espmissingincludes.h"

#define BTNGPIO    0        // input button
#define LEDGPIO    2		// Use GPIO 2 for the LED

#define MUXGPIO0  12		// Use GPIO 12
#define MUXGPIO1  13		// Use GPIO 13
#define MUXGPIO2  14		// Use GPIO 14
#define MUXGPIO3  15		// Use GPIO 15

static ETSTimer resetBtntimer;
// ----------------------------------------------------------------------------
// gpio_output_set is overkill. ToDo: use better mactos
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR ioMux(int mux)
{
    if(mux) {
        GPIO_OUTPUT_SET(MUXGPIO0, 1);
        GPIO_OUTPUT_SET(MUXGPIO1, 1);
        GPIO_OUTPUT_SET(MUXGPIO2, 1);
        GPIO_OUTPUT_SET(MUXGPIO3, 1);
    }
    else {
       GPIO_OUTPUT_SET(MUXGPIO0, 0);    
       GPIO_OUTPUT_SET(MUXGPIO1, 0);    
       GPIO_OUTPUT_SET(MUXGPIO2, 0);    
       GPIO_OUTPUT_SET(MUXGPIO3, 0);    
    } 
}
   
// ----------------------------------------------------------------------------
// gpio_output_set is overkill. ToDo: use better mactos
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR ioLed(int ena)
{
    if(ena) GPIO_OUTPUT_SET(LEDGPIO, 1);
    else    GPIO_OUTPUT_SET(LEDGPIO, 0);
}

// ----------------------------------------------------------------------------
// Button input IO. If button is pressed, GPIO0 is low, if held pressed
// for 3sec, then released, it will switch the system to dual mode.
// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR resetBtnTimerCb(void *arg)
{
    static int resetCnt = 0;
    if(!GPIO_INPUT_GET(BTNGPIO)) {        // if button is low, then reset count
        resetCnt++;
    }
    else {                                // not button is high
        if(resetCnt >= 6) {               // 3 sec pressed
            wifi_station_disconnect();    // disconnect station
            wifi_set_opmode(0x3);         // reset to AP+STA mode
            os_printf("Reset to AP mode. Restarting system...\n");
            system_restart();             // restart in dual mode
        }
        resetCnt = 0;
    }
}

// ----------------------------------------------------------------------------
// GPIO Initialization
// ----------------------------------------------------------------------------
void ioInit(void)
{
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U,  FUNC_GPIO15);   // Use MTMS pin as GPIO15
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U,  FUNC_GPIO14);   // Use MTMS pin as GPIO14
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U,  FUNC_GPIO13);   // Use MTCK pin as GPIO13
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,  FUNC_GPIO12);   // Use MTDI pin as GPIO12
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);    // Use GPIO2 
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);    // Use GPIO0
    
    gpio_output_set(0, 0, (1 << LEDGPIO), (1 << BTNGPIO));

    ioMux(0);               // switch to Debug RS232 Port

    os_timer_disarm(&resetBtntimer);
    os_timer_setfn(&resetBtntimer, resetBtnTimerCb, NULL);
    os_timer_arm(&resetBtntimer, 500, 1);
}

// ----------------------------------------------------------------------------

