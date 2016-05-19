// ----------------------------------------------------------------------------
// Basic IO Routines
// ----------------------------------------------------------------------------
#include <ets_sys.h>
#include <osapi.h>
#include <c_types.h>
#include <gpio.h>
#include <os_type.h>
#include <user_interface.h>
#include "espmissingincludes.h"
#include "io.h"
#include "client.h"

// ----------------------------------------------------------------------------
// Global definitions and variables
// ----------------------------------------------------------------------------
#define RESETBTN   0        // input button
#define RECONBTN   5        // input button
#define TESTLED    2        // Use GPIO 2 for the TEST LED

static ETSTimer resetBtntimer;

// ----------------------------------------------------------------------------
// gpio_output_set is overkill. ToDo: use better mactos
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR ioLed(int ena)
{
    if(ena) GPIO_OUTPUT_SET(TESTLED, 1);
    else    GPIO_OUTPUT_SET(TESTLED, 0);
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
            client_order();
        }
        reconCnt = 0;
    }
}

// ----------------------------------------------------------------------------
// GPIO Initialization
// ----------------------------------------------------------------------------
void ioInit(void)
{
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);    // Use GPIO5
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);    // Use GPIO4
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);    // Use GPIO2
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);    // Use GPIO0
    PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);                  // Pullup enabled on GPIO4
    PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);                  // Pullup enabled on GPIO5

    gpio_output_set(0,0,  (1 << TESTLED), 0); // Set TESTLED GPIO to output
    gpio_output_set(0,0,0,(1 << RESETBTN)  ); // Set GPIO0 as Reset
    gpio_output_set(0,0,0,(1 << RECONBTN)  ); // Set GPIO0 as Reset

    os_timer_disarm(&resetBtntimer);
    os_timer_setfn(&resetBtntimer, resetBtnTimerCb, NULL);
    os_timer_arm(&resetBtntimer, 500, 1);
}

// ----------------------------------------------------------------------------


