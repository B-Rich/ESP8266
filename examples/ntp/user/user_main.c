// ----------------------------------------------------------------------------
// NTP Test Program:
// This is a simple program to get NTP Network time using the ESP8266
// and uses a simple UART interface.
// ----------------------------------------------------------------------------
#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <user_interface.h>
#include <espconn.h>
#include "espmissingincludes.h"
#include "io.h"
#include "ntp.h"

// ----------------------------------------------------------------------------
// Global Definitions and Constants:
// ----------------------------------------------------------------------------
#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t  user_procTaskQueue[user_procTaskQueueLen];

static void loop(os_event_t *events);
static void wifi_check_ip(void *arg);

static ETSTimer WiFiLinker;

typedef enum {
        WIFI_CONNECTING,
        WIFI_CONNECTING_ERROR,
        WIFI_CONNECTED,
} tConnState;

static tConnState connState = WIFI_CONNECTING;
struct espconn Conn;
esp_udp ConnUDP;

// ----------------------------------------------------------------------------
static void wifi_check_ip(void *arg)
{
    struct ip_info ipConfig;
    os_timer_disarm(&WiFiLinker);
    switch(wifi_station_get_connect_status()) {
        case STATION_GOT_IP:
            wifi_get_ip_info(STATION_IF, &ipConfig);
            if(ipConfig.ip.addr != 0) {
                connState = WIFI_CONNECTED;
                os_printf(IPSTR, IP2STR(&ipConfig.ip));
                os_printf(" - WiFi connected\n");
                return;
            }
            break;
        case STATION_WRONG_PASSWORD:
            connState = WIFI_CONNECTING_ERROR;
            os_printf("WiFi connecting error, wrong password\n");
            break;
        case STATION_NO_AP_FOUND:
            connState = WIFI_CONNECTING_ERROR;
            os_printf("WiFi connecting error, ap not found\n");
            break;
        case STATION_CONNECT_FAIL:
            connState = WIFI_CONNECTING_ERROR;
            os_printf("WiFi connecting fail\n");
            break;
        default:
            connState = WIFI_CONNECTING;
            os_printf("WiFi connecting...\n");
            break;
    }
    os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
    os_timer_arm(&WiFiLinker, 1000, 0);
}

// ----------------------------------------------------------------------------
// Set up and connect for STA Mode
// ----------------------------------------------------------------------------
static void connect_st_mode(void)
{
    struct station_config stconfig;
    wifi_set_opmode(STATION_MODE);
    wifi_station_disconnect();       // just in case
    wifi_station_dhcpc_stop();
    if(wifi_station_get_config(&stconfig))  {
        os_memset(stconfig.ssid,     0, sizeof(stconfig.ssid));
        os_memset(stconfig.password, 0, sizeof(stconfig.password));
        os_sprintf((char *)stconfig.ssid,     "%s", WIFI_CLIENTSSID);
        os_sprintf((char *)stconfig.password, "%s", WIFI_CLIENTPASSWORD);
        if(!wifi_station_set_config(&stconfig)) {
            os_printf("ESP8266 not set station config!\n");
        }
    }
    wifi_station_connect();
    wifi_station_dhcpc_start();
    os_printf("ESP8266 in STA mode configured.\n");
}

// ----------------------------------------------------------------------------
// Set up and connect for STA Mode
// ----------------------------------------------------------------------------
static void disconnect(void)
{
    wifi_station_disconnect();
    os_printf("ESP8266 disconnected.\n");
}

// ----------------------------------------------------------------------------
// Commander functions:
// ----------------------------------------------------------------------------
static void menu(void)
{
    os_printf("\n\rNTP Test:\n");
    os_printf("lL- LED on/off\n");
    os_printf("a - Setup SoftAP mode\n");
    os_printf("c - Wifi Connect\n");
    os_printf("d - Wifi Disconnect\n");
    os_printf("i - Check IP\n");
    os_printf("n - NTP time\n");
}

// ----------------------------------------------------------------------------
// Commander functions:
// ----------------------------------------------------------------------------
static void command(char cmd)
{
    switch(cmd) {
        case 'l': ioLed(0);                     break;  // Turn test LED off
        case 'L': ioLed(1);                     break;  // Turn test LED off
        case 'h': menu();                       break;  // show menu
        case 'd': disconnect();                 break;  // disconnect station
        case 'c': connect_st_mode();            break;  // connect
        case 'i': wifi_check_ip(NULL);          break;  // Test for IP and send
        case 'n': ntp_get_time();               break;  // send test message
        default:                                break;
    }
}

// ----------------------------------------------------------------------------
// Main function
// ----------------------------------------------------------------------------
static void loop(os_event_t *events)
{
    char cmd;
    if(kbhit())  {
       os_printf("\n>");
       cmd = getch();
       stdoutPutchar(cmd);   // echo back
       stdoutPutchar('\n');  // CR
       command(cmd);
    }
    system_os_post(user_procTaskPrio, 0, 0 );
}

// ----------------------------------------------------------------------------
// Init function
// ----------------------------------------------------------------------------
void user_init(void)
{
    ioInit();               // initialize UART, GPIO and crap like that
    for(int i = 0; i < 30; i++) os_printf(" \n");
    menu();

    // ------------------------------------------------------------------------
    // Start os task
    // ------------------------------------------------------------------------
    system_os_task(loop, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0 );
}

// ----------------------------------------------------------------------------

