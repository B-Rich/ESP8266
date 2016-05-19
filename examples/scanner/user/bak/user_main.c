// ----------------------------------------------------------------------------
// yifi scanner:
// This is a simple commander program that allows the ESP8266 to be setup
// and controlled from a simple UART interface:
// ----------------------------------------------------------------------------
#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <user_interface.h>
#include <espconn.h>
#include "espmissingincludes.h"
#include "io.h"

// ----------------------------------------------------------------------------
// Global Definitions and Constants:
// ----------------------------------------------------------------------------
#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t  user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);

static uint8_t scanInProgress = 0;

// ----------------------------------------------------------------------------
// Callback the code calls when a wlan ap scan is done. Basically stores the result in
// the cgiWifiAps struct.
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR wifiScanDoneCb(void *arg, STATUS status)
{
    int n, noAps;
    struct bss_info *bss_link = (struct bss_info *)arg;

    os_printf("wifi Scan Done %d\n", status);
    if(status != OK) {
        scanInProgress = 0;
        return;
    }
    noAps = 0;
    while(bss_link != NULL) {         // Count amount of access points found.
        bss_link = bss_link->next.stqe_next;
        noAps++;
    }
    os_printf("Scan done: found %d AP's\n", noAps);
    os_printf("BSSID, SSID, RSSI, Auth\n");
    n = 0;
    bss_link = (struct bss_info *)arg;
    while(bss_link != NULL) {
        if(n >= noAps) {
            // This means the bss_link changed under our nose. Shouldn't happen!
            // Break because otherwise we will write in unallocated memory.
            os_printf("Warning: I have more than the allocated %d aps!\n", noAps);
            break;
        }
        if(bss_link->rssi != 0) {
           os_printf("%02X:%02X:%02X:%02X:%02X:%02X, ",bss_link->bssid[0],bss_link->bssid[1],bss_link->bssid[2],bss_link->bssid[3],bss_link->bssid[4],bss_link->bssid[5]);
           os_printf("%s, %d, %x\n", (char*)bss_link->ssid, bss_link->rssi,  bss_link->authmode);
        }
        bss_link = bss_link->next.stqe_next;
        n++;
    }
    scanInProgress = 0;   // done.
    os_printf("done listing APs\n");
}

// ----------------------------------------------------------------------------
// Routine to start a WiFi access point scan.
// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR wifiStartScan(void)
{
    if(scanInProgress) return;
    scanInProgress = 1;
    wifi_station_scan(NULL, wifiScanDoneCb);
}

// ----------------------------------------------------------------------------
// Commander functions:
// ----------------------------------------------------------------------------
static void menu(void)
{
    os_printf("\n\nyifi scanner:\n");
    os_printf("Ll- LED on/off\n");
    os_printf("s - Scan for AP's\n");
}

// ----------------------------------------------------------------------------
// Commander functions:
// ----------------------------------------------------------------------------
static void command(char cmd)
{
    switch(cmd) {
        case 'h': menu();                       break;  // show menu
        case 'l': ioLed(0);                     break;  // Turn test LED off
        case 'L': ioLed(1);                     break;  // Turn test LED on
        case 's': wifiStartScan();              break;  // Scan for AP's
        default:                                break;  // default
    }
}

// ----------------------------------------------------------------------------
// Main function
// ----------------------------------------------------------------------------
static void loop(os_event_t *events)
{
    if(kbhit())  {
       char cmd = getch();
       os_printf("%c\n> ", cmd);
       command(cmd);
       os_printf("\n> ");
    }
    system_os_post(user_procTaskPrio, 0, 0 );
}

// ----------------------------------------------------------------------------
// Init function
// ----------------------------------------------------------------------------
void user_init(void)
{
    ioInit();               // initialize UART, GPIO and etc
    os_delay_us(5000);
    for(int i = 0; i < 30; i++) os_printf(" \n");
    menu();
    os_printf("\n>");

    // ------------------------------------------------------------------------
    // Start os task
    // ------------------------------------------------------------------------
    system_os_task(loop, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0);
}

// ----------------------------------------------------------------------------

