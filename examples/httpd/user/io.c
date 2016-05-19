// ----------------------------------------------------------------------------
// Basic IO Routines
// ----------------------------------------------------------------------------
#include <ets_sys.h>
#include <c_types.h>
#include <osapi.h>
#include <user_interface.h>
#include <espconn.h>
#include <mem.h>
#include "espmissingincludes.h"
#include "gpio.h"
#include "stdout.h"
#include "client.h"

// ----------------------------------------------------------------------------
// Global definitions and variables
// ----------------------------------------------------------------------------
#define RESETBTN   0        // input button
#define TESTLED    2        // Use GPIO 2 for the TEST LED
#define RECONBTN   4        // input button

#define NCONFIG   12        // Use GPIO12  for the FPGA NCONFIG
#define FPGAC_CLK 13        // Use GPIO13 for DCLK
#define FPGAC_D0  15        // Use GPIO15 for DATA

static ETSTimer resetBtntimer;

// ----------------------------------------------------------------------------
// gpio_output_set is overkill. ToDo: use better mactos
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR ioMux(int mux)
{
    if(mux) nodebugmsgs = true;
    else    nodebugmsgs = false;
}

// ----------------------------------------------------------------------------
// Encapsulate gpio_output_set Macro
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR SetnConfig(int ena)
{
    if(ena) GPIO_OUTPUT_SET(NCONFIG, 1);
    else    GPIO_OUTPUT_SET(NCONFIG, 0);
}

// ----------------------------------------------------------------------------
// Encapsulate gpio_output_set Macro
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR ioLed(int ena)
{
    if(ena) GPIO_OUTPUT_SET(TESTLED, 1);
    else    GPIO_OUTPUT_SET(TESTLED, 0);
}

//--------------------------------------------------------------------------
// LOAD FPGA Config Byte - Uploads a single byte of data to FPGA
//--------------------------------------------------------------------------
void LoadFPGAByte(unsigned char Sdata) 
{
    unsigned char data = Sdata;
    for(int i = 0; i < 8; i++) {
        if(data&1) GPIO_OUTPUT_SET(FPGAC_D0, 1);
        else       GPIO_OUTPUT_SET(FPGAC_D0, 0);
        GPIO_OUTPUT_SET(FPGAC_CLK, 0);
        GPIO_OUTPUT_SET(FPGAC_CLK, 1);
        data = data>>1;
    }
}

// ----------------------------------------------------------------------------
// Button input IO. If button is pressed, GPIO0 is low, if held pressed
// for 3sec, then released, it will switch the system to dual mode.
// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR resetBtnTimerCb(void *arg)
{
    static int resetCnt = 0;
    static int reconCnt = 0;

    if(!GPIO_INPUT_GET(RESETBTN)) {       // if button is low, then count
        resetCnt++;
    }
    else {                                // not button is high again
        if(resetCnt >= 6) {               // 10 sec pressed
            wifi_station_disconnect();    // disconnect station
            wifi_set_opmode(0x3);         // reset to AP+STA mode
            os_printf("Reset to STA+AP mode. Restarting system...\n");
            system_restart();             // restart in dual mode
        }
        else if(resetCnt >= 1) {          // 1 sec pressed
            os_printf("Reconnect...\n");
            wifi_station_disconnect();    // disconnect station
            wifi_station_connect();       // reconnect
        }
        resetCnt = 0;
    }

    if(!GPIO_INPUT_GET(RECONBTN)) {       // if button is low, then count
        reconCnt++;
    }
    else {                                // not button is high again
        if(reconCnt >= 1) {               // pressed for 1 sec
            client_order();               // Perform client ordering function
        }
        reconCnt = 0;
    }

}

// ----------------------------------------------------------------------------
// GPIO Initialization
// ----------------------------------------------------------------------------
void ioInit(void)
{
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);    // Use GPIO0
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);    // Use GPIO2
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);    // Use GPIO4
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);    // Use GPIO5

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,  FUNC_GPIO12);   // Use MTDI pin as GPIO12
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U,  FUNC_GPIO13);   // Use MTCK pin as GPIO13
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U,  FUNC_GPIO14);   // Use MTMS pin as GPIO14
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U,  FUNC_GPIO15);   // Use MTMS pin as GPIO15
    
    PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);                  // Pullup enabled on GPIO4
    PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);                  // Pullup enabled on GPIO5

    gpio_output_set(0,0,  (1 << TESTLED), 0); // Set TESTLED GPIO to output
    gpio_output_set(0,0,  (1 << NCONFIG), 0); // Set BUSYLED GPIO to output
    gpio_output_set(0,0,0,(1 << RESETBTN)  ); // Set GPIO2 as Reset
    gpio_output_set(0,0,0,(1 << RECONBTN)  ); // Set GPIO4 as connect

    ioMux(0);                                 // switch to Debug mode
    SetnConfig(1);

    os_timer_disarm(&resetBtntimer);
    os_timer_setfn(&resetBtntimer, resetBtnTimerCb, NULL);
    os_timer_arm(&resetBtntimer, 500, 1);
}

// ----------------------------------------------------------------------------

