// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// wifi scanner:
// This is a simple program that uses the ESP8266 to scan for wifi SSID's
// and reports the BSSID, SSID and RSSI for each AP it finds.
// ----------------------------------------------------------------------------
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
#define user_procTaskPrio        0      // process task priority
#define user_procTaskQueueLen    1      // process task queue len (single thread)
os_event_t  user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);

static uint8_t scanInProgress = 0;      // flag to prevent calling scan more than one at a time

// ----------------------------------------------------------------------------
// Callback Function.
// When you call the function wifi_station_scan() below, the ESP8266 goes off
// and starts scanning. When it is done, this function is called automatically.
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR wifiScanDoneCb(void *arg, STATUS status)
{
    int n, noAps;
    struct bss_info *bss_link = (struct bss_info *)arg;

    os_printf("wifi Scan Done %d\n", status);
    if(status != OK) {
        scanInProgress = 0;   // clear scanning in progress flag
        return;
    }
    noAps = 0;                         // variable for count of AP's found
    while(bss_link != NULL) {          // Count amount of access points found.
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
        bss_link = bss_link->next.stqe_next;  // linked list, step to next link
        n++;
    }
    scanInProgress = 0;                 // clear scanning in progress flag
    os_printf("done listing APs\n");
}

// ----------------------------------------------------------------------------
// Routine to start a WiFi access point scan.
// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR wifiStartScan(void)
{
    if(scanInProgress) return;               // if we called it before, then dont do that until it is done
    scanInProgress = 1;                      // Set the flag to make sure we dont call it twice
    wifi_station_scan(NULL, wifiScanDoneCb); // kicks things off, when done, the Cb function is called
}

// ----------------------------------------------------------------------------
// Main Menu Display:
// ----------------------------------------------------------------------------
static void menu(void)
{
    os_printf("\n\nwifi scanner:\n");
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
// Main function Loop
// ----------------------------------------------------------------------------
static void loop(os_event_t *events)
{
    if(kbhit())  {
       char cmd = getch();
       os_printf("%c\n> ", cmd);
       command(cmd);
       os_printf("\n> ");
    }
    system_os_post(user_procTaskPrio, 0, 0); // Post to process queue
}

// ----------------------------------------------------------------------------
// Init function
// ----------------------------------------------------------------------------
void user_init(void)
{
    ioInit();               // initialize UART, GPIO and etc
    os_delay_us(5000);      // wait 5ms
    for(int i = 0; i < 30; i++) os_printf(" \n");  // scrolls down the terminal
    menu();                                        // show the menu
    os_printf("\n>");                              // user prompt

    // ------------------------------------------------------------------------
    // Start os task
    // ------------------------------------------------------------------------
    system_os_task(loop, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0);
}

// ----------------------------------------------------------------------------

