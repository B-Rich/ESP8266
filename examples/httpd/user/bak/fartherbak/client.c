// ----------------------------------------------------------------------------
// Example of to get and send data to a web site
// ----------------------------------------------------------------------------
#include <ets_sys.h>
#include <osapi.h>
#include <httpd.h>
#include <c_types.h>
#include <mem.h>
#include <os_type.h>
#include <user_interface.h>
#include "espmissingincludes.h"
#include "user_config.h"
#include "contents.h"
#include "httpclient.h"
#include "client.h"

// ----------------------------------------------------------------------------
// Global Definitions and Constants:
// ----------------------------------------------------------------------------
typedef enum {
    WIFI_CONNECTING,
    WIFI_CONNECTING_ERROR,
    WIFI_CONNECTED,
} tConnState;

static struct ip_info ipConfig;
static tConnState connState = WIFI_CONNECTING;

const char *WiFiStatus[] =
{
    "STATION_IDLE",             // 0x00
    "STATION_CONNECTING",       // 0x01
    "STATION_WRONG_PASSWORD",   // 0x02
    "STATION_NO_AP_FOUND",      // 0x03
    "STATION_CONNECT_FAIL",     // 0x04
    "STATION_GOT_IP"            // 0x05
};

// ----------------------------------------------------------------------------
/*
static void data_get(void)
{
    static char url[256];

    switch(connState)  {
        case WIFI_CONNECTED:
            wifi_get_ip_info(STATION_IF, &ipConfig);

            // Start the connection process
            os_sprintf(url, "http://%s:8086", DATA_SERVER);
            os_printf("Request: %s\r\n", url);
            http_get(url, data_http_callback);
            break;
        default:
            os_printf("WiFi not connected...\r\n");
            break;
    }
}
*/

// ----------------------------------------------------------------------------
// check for ip connection
// ----------------------------------------------------------------------------
static void wifi_check_ip(void)
{
    switch(wifi_station_get_connect_status())     {
        case STATION_GOT_IP:
            wifi_get_ip_info(STATION_IF, &ipConfig);
            if(ipConfig.ip.addr != 0) {
                connState = WIFI_CONNECTED;
                os_printf("WiFi connected...\r\n");
            }
            else {
                connState = WIFI_CONNECTING_ERROR;
                os_printf("WiFi connected, ip.addr is null\r\n");
            }
            break;
        case STATION_WRONG_PASSWORD:
            connState = WIFI_CONNECTING_ERROR;
            os_printf("WiFi connecting error, wrong password\r\n");
            break;
        case STATION_NO_AP_FOUND:
            connState = WIFI_CONNECTING_ERROR;
            os_printf("WiFi connecting error, ap not found\r\n");
            break;
        case STATION_CONNECT_FAIL:
            connState = WIFI_CONNECTING_ERROR;
            os_printf("WiFi connecting fail\r\n");
            break;
        default:
            connState = WIFI_CONNECTING;
            os_printf("WiFi connecting...\r\n");
            break;
    }
}

// ----------------------------------------------------------------------------
// Client Init function
// ----------------------------------------------------------------------------
void client_init(void)
{
    os_printf("client initialization\n\r");
    wifi_station_connect();
    wifi_station_dhcpc_start();
    wifi_station_set_auto_connect(1);
}

// ----------------------------------------------------------------------------
static void client_http_callback(char *response, int http_status, char *full_response)
{
    os_printf("Response:\r\n");
    if(http_status == 200) {
        os_printf("\r\nresponse = %s\r\n", response);
        os_printf("-----------------\r\n");
    }
    else {
        os_printf("http_status = %d\r\n", http_status);
        os_printf("strlen of response = %d\r\n", strlen(response) );
        os_printf("strlen of full_response = %d\r\n", strlen(full_response) );
        os_printf("response = %s\r\n", response);
        os_printf("-----------------\r\n");
    }
}

// ----------------------------------------------------------------------------
// post json data to
// ----------------------------------------------------------------------------
static void client_post(char *data)
{
    static char url[256];
    static int msgnum = 0;

    #ifdef HTTP_DEBUG_ON
        os_printf("HTTP Post\n");
    #endif
    msgnum++;
    wifi_get_ip_info(STATION_IF, &ipConfig);
    os_sprintf(url,"%s", DATA_SERVER);       // Start the connection process
    os_printf("msg = %d\n", msgnum);
    os_printf("Request: %s\n%s\n", url, data);
    http_post(url, data, client_http_callback);
    #ifdef HTTP_DEBUG_ON
        os_printf("HTTP Post Completed\r\n");
    #endif
}

// ----------------------------------------------------------------------------
// Test JSON string
// ----------------------------------------------------------------------------
static char *cont_jsn =
"json={\"user\":\"donna\",\"pass\":\"12345\",\"contents\":["
"{\"name\":\"Vodka\",\"amount\":750},"
"{\"name\":\"Lemonade\",\"amount\":750},"
"{\"name\":\"OJ\",\"amount\":750},"
"{\"name\":\"Schnapps\",\"amount\":750},"
"{\"name\":\"Juice\",\"amount\":750},"
"{\"name\":\"Whiskey\",\"amount\":750},"
"{\"name\":\"Grenadine\",\"amount\":750},"
"{\"name\":\"Soda\",\"amount\":750},"
"{\"name\":\"Triple Sec\",\"amount\":750}"
"]}";

// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR client_order(void)
{
//  char buffer[1024] = "json=";

    wifi_check_ip();                       // Test for IP and connected
    if(connState == WIFI_CONNECTED) {
//      get_contents(&buffer[5]);          // get contetents
//      data_post(buffer);
        client_post(cont_jsn);
    }
    else {
        os_printf("WiFi not connected...\r\n");
    }

}

// ----------------------------------------------------------------------------

