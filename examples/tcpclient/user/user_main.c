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
#include "io.h"

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
static uint8_t continuoussend = 0;

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

// ----------------------------------------------------------------------------
static void tcpclient_discon_cb(void *arg)
{
    struct espconn *pespconn = arg; ;
    connState = TCP_DISCONNECTED;
    os_printf("tcpclient_discon_cb\n");
    if(pespconn == NULL){
        os_printf("tcpclient_discon_cb - conn is NULL!\n");
        return;
    }
    if(continuoussend) {
        os_printf("Re-connect in 5s...\n");
        os_timer_disarm(&WiFiLinker);
        os_timer_setfn(&WiFiLinker, (os_timer_func_t *)platform_reconnect, pespconn);
        os_timer_arm(&WiFiLinker, 5000, 0);
    }
}

// ----------------------------------------------------------------------------
static void tcpclient_sent_cb(void *arg)
{
    struct espconn *pespconn = arg;
    os_printf("tcpclient_sent_cb\n");
    espconn_disconnect(pespconn);
}

// ----------------------------------------------------------------------------
static void tcpclient_recon_cb(void *arg, sint8 err)
{
    struct espconn *pespconn = arg;
    connState = TCP_DISCONNECTED;

    os_printf("tcpclient_recon_cb\n");
    if(err != ESPCONN_OK) {
        os_printf("Connection error: %d - %s\r\n", err, ((err>-16)&&(err<1))? sEspconnErr[-err] : "?");
    }
    if(++tcpReconCount >= 5)  {
        connState = TCP_CONNECTING_ERROR;
        tcpReconCount = 0;
        os_printf("tcpclient_recon_cb, 5 failed TCP attempts!\n");
        os_printf("Reconnecing in 10s...\n");
        os_timer_disarm(&WiFiLinker);
        os_timer_setfn(&WiFiLinker, (os_timer_func_t *)platform_reconnect, pespconn);
        os_timer_arm(&WiFiLinker, 10000, 0);
    }
    else {
        os_printf("Reconnecing in 2s...\n");
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

    os_printf("tcpclient_connect_cb\n");
    espconn_regist_sentcb(pespconn, tcpclient_sent_cb);
    connState = TCP_CONNECTED;
    os_sprintf(payload, "ESP8266 Msg# %d", msgcnt++);
    sint8 espsent_status = espconn_sent(pespconn, (uint8 *)payload, strlen(payload));
    if(espsent_status == ESPCONN_OK) {
        connState = TCP_SENT_DATA;
        os_printf("Data sent, payload: %s\n", payload);
    }
    else {
        connState = TCP_SENDING_DATA_ERROR;
        os_printf("Error while sending data.\n");
    }
}

// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR platform_reconnect(struct espconn *pespconn)
{
    os_printf("platform_reconnect\n");
    wifi_check_ip(NULL);
}

// ----------------------------------------------------------------------------
static void senddata(void)
{
    os_timer_disarm(&WiFiLinker);
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
    os_printf("Start espconn_connect to " IPSTR ":%d\n", IP2STR(Conn.proto.tcp->remote_ip), Conn.proto.tcp->remote_port);
    sint8 espcon_status = espconn_connect(&Conn);
    switch(espcon_status) {
        case ESPCONN_OK:      os_printf("TCP created.\n");                        break;
        case ESPCONN_RTE:     os_printf("Error connection, routing problem.\n");  break;
        case ESPCONN_TIMEOUT: os_printf("Error connection, timeout.\n");          break;
        default:              os_printf("Connection error: %d\n", espcon_status); break;
    }
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
                os_printf("WiFi connected\n");
                os_printf("Start TCP connecting...\n");
                connState = TCP_CONNECTING;
                senddata();
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
// Commander functions:
// ----------------------------------------------------------------------------
static void menu(void)
{
    os_printf("\n\ryifi commander:\n");
    os_printf("l - Toggle LED on/off\n");
    os_printf("c - Wifi Connect\n");
    os_printf("d - Wifi Disconnect\n");
    os_printf("s - TCP Send one message\n");
    os_printf("e - Continuous send enabled\n");
    os_printf("n - Continuous send disabled\n");
}

// ----------------------------------------------------------------------------
// Commander functions:
// ----------------------------------------------------------------------------
static void command(char cmd)
{
    switch(cmd) {
        case 'h': menu();                    break;  // show menu
        case 'l': ioLed(2);                  break;  // Turn test LED off
        case 'd': disconnect();              break;  // disconnect station
        case 'c': connect_st_mode();         break;  // connect
        case 's': wifi_check_ip(NULL);       break;  // Test for IP and send
        case 'e': continuoussend = 1;        break;  // Enable continuous send
        case 'n': continuoussend = 0;        break;  // Disable continuous send
        default:                             break;
    }
}

// ----------------------------------------------------------------------------
// Main function
// ----------------------------------------------------------------------------
static void loop(os_event_t *events)
{
    if(kbhit())  {
       char cmd = getch();
       stdoutPutchar(cmd);   // echo back
       stdoutPutchar('\n');  // CR
       command(cmd);
       stdoutPutchar('>');
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
    for(int i = 0; i < 30; i++) os_printf(" \r\n");
    os_printf("\n\ryifi commander: (hit h for menu)\n\r");
    stdoutPutchar('>');
    system_os_task(loop, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, 0, 0 );
}

// ----------------------------------------------------------------------------

