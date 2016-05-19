// ----------------------------------------------------------------------------
// This is a simple program that will us the ESP8266 to log data to a simple
// UDP server. The data logged is the list of AP's
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

#ifdef DEBUG_MSGS

const char *sEspconnErr[] =
{
    "Ok",                    // ERR_OK          0
    "Out of memory error",   // ERR_MEM        -1
    "Buffer error",          // ERR_BUF        -2
    "Timeout",               // ERR_TIMEOUT    -3
    "Routing problem",       // ERR_RTE        -4
    "Operation in progress", // ERR_INPROGRESS -5
    "Illegal value",         // ERR_VAL        -6
    "Operation would block", // ERR_WOULDBLOCK -7
    "Connection aborted",    // ERR_ABRT       -8
    "Connection reset",      // ERR_RST        -9
    "Connection closed",     // ERR_CLSD       -10
    "Not connected",         // ERR_CONN       -11
    "Illegal argument",      // ERR_ARG        -12
    "Address in use",        // ERR_USE        -13
    "Low-level netif error", // ERR_IF         -14
    "Already connected"      // ERR_ISCONN     -15
};
#endif

// ----------------------------------------------------------------------------
// flag to prevent calling scan more than one at a time
// ----------------------------------------------------------------------------
static uint8_t scanInProgress = 0;

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
static void wifi_check_ip(void *arg)
{
    struct ip_info ipConfig;
    os_timer_disarm(&WiFiLinker);
    switch(wifi_station_get_connect_status()) {
        case STATION_GOT_IP:
            wifi_get_ip_info(STATION_IF, &ipConfig);
            if(ipConfig.ip.addr != 0) {
                connState = WIFI_CONNECTED;
                os_printf("WiFi connected\n");
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
void connect_st_mode(void)
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
void disconnect(void)
{
    wifi_station_disconnect();
    os_printf("ESP8266 disconnected.\n");
}

// ----------------------------------------------------------------------------
// Expecting command:
//  01234567890
// "Pour(ml) = 20"    will pour 20ml of liquid
// ----------------------------------------------------------------------------
static void recv_cb(void *arg, char *pusrdata, unsigned short length)
{
     os_printf("recv udp data: [%s]\n", pusrdata);

     if(pusrdata[0] == 'P') {
        char *amount = &pusrdata[1];
        pusrdata[6] = 0; 
        int ml = atoi(amount);
        int timems = ml * 10;
        os_printf("motor on\n");
        ioMotor(1);
        for(int k = 0; k < timems; k++) os_delay_us(1000);
        ioMotor(0);
        os_printf("motor off\n");
     }
}

// ----------------------------------------------------------------------------
static void sent_cb(void *arg)
{
    struct espconn *pespconn = arg;
    os_printf("sent done\n");
    espconn_disconnect(pespconn);
}

// ----------------------------------------------------------------------------
// Send data
// Parameters:  uint8* psent -- Data to send
//              uint16 length -- Length of data to send
// ----------------------------------------------------------------------------
static void create_udp(void)
{
    uint32_t ip;

    os_timer_disarm(&WiFiLinker);

    ip = ipaddr_addr(UDPSERVERIP);
    os_memcpy(ConnUDP.remote_ip, &ip, 4);

    struct ip_info ipConfig;
    wifi_get_ip_info(STATION_IF, &ipConfig);
    os_memcpy(ConnUDP.local_ip, &ipConfig.ip.addr, 4);

    ConnUDP.local_port  = UDPSERVERPORT;
    ConnUDP.remote_port = UDPSERVERPORT;

    Conn.proto.udp = &ConnUDP;
    Conn.type      = ESPCONN_UDP;
    Conn.state     = ESPCONN_NONE;

    espconn_regist_recvcb(&Conn,  recv_cb); // register a udp packet receiving callback
    espconn_regist_sentcb(&Conn,  sent_cb);

    os_printf("Start espconn_connect to   " IPSTR ":%d\n", IP2STR(Conn.proto.udp->remote_ip), Conn.proto.udp->remote_port);
    os_printf("Start espconn_connect from " IPSTR ":%d\n", IP2STR(Conn.proto.udp->local_ip), Conn.proto.udp->local_port);
    espconn_create(&Conn);   // create udp
    os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
    os_timer_arm(&WiFiLinker, 1000, 0);
}

// ----------------------------------------------------------------------------
static void send_msg(void)
{
    char payload[128];
    static int msgcnt = 0;

    os_printf("UDP sending...\n");
    os_sprintf(payload, "ESP8266 Msg# %d", msgcnt++);
    espconn_sent(&Conn, (uint8 *)payload, os_strlen(payload));
}

// ----------------------------------------------------------------------------
// Commander functions:
// ----------------------------------------------------------------------------
static void menu(void)
{
    os_printf("\n\ryifi commander:\n");
    os_printf("l - LED on/off\n");
    os_printf("mM- Motor on/off\n\r");
    os_printf("c - Wifi Connect\n");
    os_printf("d - Wifi Disconnect\n");
    os_printf("i - Check IP\n");
    os_printf("u - create UDP\n");
    os_printf("s - Send test msg\n");
    os_printf("w - Scan for AP's\n");
}

// ----------------------------------------------------------------------------
// Commander functions:
// ----------------------------------------------------------------------------
static void command(char cmd)
{
    switch(cmd) {
        case 'l': ioLed(2);                     break;  // Turn test LED off
        case 'm': ioMotor(0);                   break;  // Turn Motor off
        case 'M': ioMotor(1);                   break;  // Turn Motor on
        case 'h': menu();                       break;  // show menu
        case 'd': disconnect();                 break;  // disconnect station
        case 'c': connect_st_mode();            break;  // connect
        case 'i': wifi_check_ip(NULL);          break;  // Test for IP and send
        case 'u': create_udp();                 break;  // send test message
        case 's': send_msg();                   break;  // send test message
        case 'w': wifiStartScan();              break;  // Scan for AP's
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
       stdoutPutchar(cmd);  // echo back
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
    os_delay_us(10000);
    for(int i = 0; i < 30; i++) os_printf(" \n");
    menu();

    // ------------------------------------------------------------------------
    // Start os task
    // ------------------------------------------------------------------------
    system_os_task(loop, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0 );
}

// ----------------------------------------------------------------------------

