// ----------------------------------------------------------------------------
// yifi Commander:
// This is a simple commander program that allows the ESP8266 to be setup
// and controlled from a simple UART interface:
// ----------------------------------------------------------------------------
#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <user_interface.h>
#include <espconn.h>
#include "espmissingincludes.h"
#include "user_config.h"
#include "io.h"

//extern int ets_uart_printf(const char *fmt, ...);
//int (*con_printf)(const char *fmt, ...) = ets_uart_printf;

// ----------------------------------------------------------------------------
// Global Definitions and Constants:
// ----------------------------------------------------------------------------
#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t  user_procTaskQueue[user_procTaskQueueLen];

static void loop(os_event_t *events);
static void wifi_check_ip(void *arg);
static void platform_reconnect(struct espconn *);
static ETSTimer WiFiLinker;
typedef enum {
        WIFI_CONNECTING,
        WIFI_CONNECTING_ERROR,
        WIFI_CONNECTED,
        TCP_DISCONNECTED,
        TCP_CONNECTING,
        TCP_CONNECTING_ERROR,
        TCP_CONNECTED,
        TCP_SENDING_DATA_ERROR,
        TCP_SENT_DATA
} tConnState;
static tConnState connState = WIFI_CONNECTING;
struct espconn Conn;
esp_tcp ConnTcp;
static unsigned char tcpReconCount;

#ifdef DEBUG_MSGS
// enum espconn state, see file /include/lwip/api/err.c
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
static void tcpclient_discon_cb(void *arg)
{
    struct espconn *pespconn = arg; ;
    connState = TCP_DISCONNECTED;
    #ifdef DEBUG_MSGS
        os_printf("tcpclient_discon_cb\r\n");
    #endif
    if(pespconn == NULL){
        #ifdef DEBUG_MSGS
            os_printf("tcpclient_discon_cb - conn is NULL!\r\n");
        #endif
        return;
    }
    #ifdef PLATFORM_DEBUG
        os_printf("Will reconnect in 2s...\r\n");
    #endif
    os_timer_disarm(&WiFiLinker);
    os_timer_setfn(&WiFiLinker, (os_timer_func_t *)platform_reconnect, pespconn);
    os_timer_arm(&WiFiLinker, 2000, 0);
}

// ----------------------------------------------------------------------------
static void tcpclient_sent_cb(void *arg)
{
    struct espconn *pespconn = arg;
    #ifdef DEBUG_MSGS
        os_printf("tcpclient_sent_cb\r\n");
    #endif
    espconn_disconnect(pespconn);
}

// ----------------------------------------------------------------------------
static void tcpclient_recon_cb(void *arg, sint8 err)
{
    struct espconn *pespconn = arg;
    connState = TCP_DISCONNECTED;

    #ifdef DEBUG_MSGS
        os_printf("tcpclient_recon_cb\r\n");
        if(err != ESPCONN_OK)
            os_printf("Connection error: %d - %s\r\n", err, ((err>-16)&&(err<1))? sEspconnErr[-err] : "?");
    #endif

    if(++tcpReconCount >= 5)  {
        connState = TCP_CONNECTING_ERROR;
        tcpReconCount = 0;
        #ifdef DEBUG_MSGS
            os_printf("tcpclient_recon_cb, 5 failed TCP attempts!\r\n");
            os_printf("Will reconnect in 10s...\r\n");
        #endif
        os_timer_disarm(&WiFiLinker);
        os_timer_setfn(&WiFiLinker, (os_timer_func_t *)platform_reconnect, pespconn);
        os_timer_arm(&WiFiLinker, 10000, 0);
    }
    else {
        #ifdef DEBUG_MSGS
            os_printf("Will reconnect in 2s...\r\n");
        #endif
        os_timer_disarm(&WiFiLinker);
        os_timer_setfn(&WiFiLinker, (os_timer_func_t *)platform_reconnect, pespconn);
        os_timer_arm(&WiFiLinker, 2000, 0);
    }
}

// ----------------------------------------------------------------------------
static void tcpclient_connect_cb(void *arg)
{
    struct espconn *pespconn = arg;
    tcpReconCount = 0;
    char payload[128];
    static int msgcnt = 0;

    #ifdef DEBUG_MSGS
        os_printf("tcpclient_connect_cb\r\n");
    #endif
    espconn_regist_sentcb(pespconn, tcpclient_sent_cb);
    connState = TCP_CONNECTED;
    os_sprintf(payload, "ESP8266 Msg# %d", msgcnt++);
    sint8 espsent_status = espconn_sent(pespconn, (uint8 *)payload, strlen(payload));
    if(espsent_status == ESPCONN_OK) {
        connState = TCP_SENT_DATA;
        #ifdef DEBUG_MSGS
            os_printf("Data sent, payload: %s\r\n", payload);
        #endif
    }
    else {
        connState = TCP_SENDING_DATA_ERROR;
        #ifdef DEBUG_MSGS
            os_printf("Error while sending data.\r\n");
        #endif
    }
}

// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR platform_reconnect(struct espconn *pespconn)
{
    #ifdef PLATFORM_DEBUG
        os_printf("platform_reconnect\r\n");
    #endif
    wifi_check_ip(NULL);
}

// ----------------------------------------------------------------------------
static void senddata(void)
{
    os_timer_disarm(&WiFiLinker);
//  char info[150];
    char tcpserverip[15];

    Conn.proto.tcp = &ConnTcp;
    Conn.type = ESPCONN_TCP;
    Conn.state = ESPCONN_NONE;
    os_sprintf(tcpserverip, "%s", TCPSERVERIP);
    uint32_t ip = ipaddr_addr(tcpserverip);
    os_memcpy(Conn.proto.tcp->remote_ip, &ip, 4);
    Conn.proto.tcp->local_port = espconn_port();
    Conn.proto.tcp->remote_port = TCPSERVERPORT;
    espconn_regist_connectcb(&Conn, tcpclient_connect_cb);
    espconn_regist_reconcb(&Conn,   tcpclient_recon_cb);
    espconn_regist_disconcb(&Conn,  tcpclient_discon_cb);
    #ifdef DEBUG_MSGS
        os_printf("Start espconn_connect to " IPSTR ":%d\r\n", IP2STR(Conn.proto.tcp->remote_ip), Conn.proto.tcp->remote_port);
    #endif
    sint8 espcon_status = espconn_connect(&Conn);
    #ifdef DEBUG_MSGS
        switch(espcon_status) {
            case ESPCONN_OK:      os_printf("TCP created.\r\n");                        break;
            case ESPCONN_RTE:     os_printf("Error connection, routing problem.\r\n");  break;
            case ESPCONN_TIMEOUT: os_printf("Error connection, timeout.\r\n");          break;
            default:              os_printf("Connection error: %d\r\n", espcon_status); break;
        }
    #endif
    if(espcon_status != ESPCONN_OK) {
        os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
        os_timer_arm(&WiFiLinker, 1000, 0);
    }
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
                #ifdef DEBUG_MSGS
                    os_printf("WiFi connected\r\n");
                    os_printf("Start TCP connecting...\r\n");
                #endif
                connState = TCP_CONNECTING;
                senddata();
                return;
            }
            break;
        case STATION_WRONG_PASSWORD:
            connState = WIFI_CONNECTING_ERROR;
            #ifdef DEBUG_MSGS
                os_printf("WiFi connecting error, wrong password\r\n");
            #endif
            break;
        case STATION_NO_AP_FOUND:
            connState = WIFI_CONNECTING_ERROR;
            #ifdef DEBUG_MSGS
                 os_printf("WiFi connecting error, ap not found\r\n");
            #endif
            break;
        case STATION_CONNECT_FAIL:
            connState = WIFI_CONNECTING_ERROR;
            #ifdef DEBUG_MSGS
                 os_printf("WiFi connecting fail\r\n");
            #endif
            break;
        default:
            connState = WIFI_CONNECTING;
            #ifdef DEBUG_MSGS
                 os_printf("WiFi connecting...\r\n");
            #endif
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
             #ifdef DEBUG_MSGS
                 os_printf("ESP8266 not set station config!\r\n");
             #endif
        }
    }
    wifi_station_connect();
    wifi_station_dhcpc_start();
//  wifi_station_set_auto_connect(1);
    #ifdef DEBUG_MSGS
        os_printf("ESP8266 in STA mode configured.\r\n");
    #endif
}

// ----------------------------------------------------------------------------
// Set up and connect for STA Mode
// ----------------------------------------------------------------------------
void disconnect(void)
{
     wifi_station_disconnect();
    #ifdef DEBUG_MSGS
        os_printf("ESP8266 disconnected.\r\n");
    #endif
}

// ----------------------------------------------------------------------------
// Commander functions:
// ----------------------------------------------------------------------------
static void menu(void)
{
    os_printf("\n\ryifi commander:\n\r");
    os_printf("  c - Wifi Connect\n\r");
    os_printf("  d - Wifi Disconnect\n\r");
    os_printf("  i - TCP Connect\n\r");
    os_printf("L/l -LED on/off\n\r");
}

// ----------------------------------------------------------------------------
// Commander functions:
// ----------------------------------------------------------------------------
static void command(char cmd)
{
    switch(cmd) {
        case 'h': menu();                       break;  // show menu
        case 'd': disconnect();                 break;  // disconnect station
        case 'c': connect_st_mode();            break;  // connect
        case 'i': wifi_check_ip(NULL);          break;  // Test for IP and send
        case 'l': ioLed(0);                     break;  // Turn test LED off
        case 'L': ioLed(1);                     break;  // Turn test LED on
        default:
            break;
    }
}

// ----------------------------------------------------------------------------
// Main function
// ----------------------------------------------------------------------------
static void loop(os_event_t *events)
{
    char cmd;
    if(kbhit())  {
       cmd = getch();
       uart_tx_one_char(cmd);  // echo back
       uart_tx_one_char(0x0d);  // CR
       uart_tx_one_char(0x0a);  // LF
       command(cmd);
    }
    ETS_UART_INTR_ENABLE();
    system_os_post(user_procTaskPrio, 0, 0 );
}

// ----------------------------------------------------------------------------
// Init function 
// ----------------------------------------------------------------------------
void user_init(void)
{
    int i;

    ioInit();               // initialize UART, GPIO and crap like that
    os_delay_us(10000);
    for(i = 0; i < 30; i++) os_printf(" \r\n");
    os_printf("\n\ryifi commander: (hit h for menu)\n\r");

    // Start os task
    system_os_task(loop, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0 );
}

// ----------------------------------------------------------------------------

