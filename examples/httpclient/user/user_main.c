// ----------------------------------------------------------------------------
// yifi Commander:
// This is a simple commander program that allows the ESP8266 to be setup
//
// Example of to get and send data to a web site
// ----------------------------------------------------------------------------
#include <osapi.h>
#include <c_types.h>
#include <mem.h>
#include <os_type.h>
#include <user_interface.h>
#include <espconn.h>
#include "espmissingincludes.h"

#include "httpclient.h"
#include "client.h"
#include "io.h"
#include "stdout.h"
#include "user_config.h"

// ----------------------------------------------------------------------------
// Global Definitions and Constants:
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Set up and connect for STA + AP Mode
// ----------------------------------------------------------------------------
void connect_stap_mode(void)
{
     struct station_config stconfig;
     wifi_set_opmode(0x3);            // reset to AP+STA mode
     wifi_station_disconnect();       // just in case
     wifi_station_dhcpc_stop();
     if(wifi_station_get_config(&stconfig))  {
         os_memset(stconfig.ssid,     0, sizeof(stconfig.ssid));
         os_memset(stconfig.password, 0, sizeof(stconfig.password));
         os_sprintf((char *)stconfig.ssid,     "%s", "ZQ097");
         os_sprintf((char *)stconfig.password, "%s", "antiquark13");
         if(!wifi_station_set_config(&stconfig)) {
             os_printf("ESP8266 not set station config!\r\n");
        }
    }
    os_printf("ESP8266 in STA mode configured.\r\n");
}

// ----------------------------------------------------------------------------
// Change my SSID
// ----------------------------------------------------------------------------
static void user_SSID(char *ssid)
{
    struct softap_config ap_cfg;
    while(!wifi_softap_get_config(&ap_cfg));
    memset(ap_cfg.ssid, 0, 32);
    os_memcpy(&ap_cfg.ssid, ssid, strlen(ssid));
    wifi_softap_set_config(&ap_cfg);
}

// ----------------------------------------------------------------------------
// Set up and connect for STA Mode
// ----------------------------------------------------------------------------
void disconnect(void)
{
    wifi_station_disconnect();
    os_printf("ESP8266 disconnected.\r\n");
}

// ----------------------------------------------------------------------------
// User Init function
// ----------------------------------------------------------------------------
void user_init(void)
{
    int i;
    stdoutInit();
    ioInit();               // initialize UART, GPIO and crap like that
    os_delay_us(10000);
    for(i = 0; i < 30; i++) os_printf(" \r\n");
    os_printf("HTTP Client Test Program.\n\r");
    user_SSID("10Forward");
//  connect_stap_mode();         // connect
    client_init();
}

// ----------------------------------------------------------------------------

