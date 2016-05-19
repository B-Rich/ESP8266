// ----------------------------------------------------------------------------
// Example of to get and send data to a web site
// ----------------------------------------------------------------------------
#include <ets_sys.h>
#include <osapi.h>
#include <c_types.h>
#include <mem.h>
#include <os_type.h>
#include <user_interface.h>
#include <json/jsonparse.h>
#include "espmissingincludes.h"
#include "user_config.h"
#include "httpclient.h"
#include "client.h"
#include "stdout.h"
#include "io.h"

// ----------------------------------------------------------------------------
// Global Definitions and Constants:
// ----------------------------------------------------------------------------
typedef enum {
    WIFI_CONNECTING,
    WIFI_CONNECTING_ERROR,
    WIFI_CONNECTED,
} tConnState;
static tConnState connState = WIFI_CONNECTING;

static struct ip_info ipConfig;

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
// list of canister pour commands executed
// ----------------------------------------------------------------------------
typedef struct {
    int amount;
    int sequence;
} PourData;
PourData pour[16];
// ----------------------------------------------------------------------------
static void pourexec(void)
{
    char buf[16];
    for(int i = 0; i < 16; i++) {
        if(pour[i].amount) {
            os_sprintf(buf,"\r>P%02d%d\r",i,pour[i].amount);
            for(int j = 0; j < strlen(buf); j++) {
                stdoutUartTxd(buf[i]);
            }
        }
    }
    os_printf("pour commands done\n");
}

// ----------------------------------------------------------------------------
// receives the json list of canister pour commands and parses
// ----------------------------------------------------------------------------
static void parsepour(char *jsdata,  int len)
{
    char type;
    char str[80];
    char buf[80];

    struct jsonparse_state state;
    struct jsonparse_state *js = &state;
    jsonparse_setup(js, jsdata, len);
    type = jsonparse_next(js);
    if(type != JSON_TYPE_OBJECT) {
        os_printf("Not a valid JSON file\n");
        return;
    }
    type = jsonparse_next(js);
    if(type != JSON_TYPE_PAIR_NAME) {
        os_printf("Not a valid JSON object\n");
        return;
    }
    jsonparse_copy_value(js, str, 80);
    if(jsonparse_strcmp_value(js, "pour") != 0) {
        os_printf("Not a valid pour list 1\n");
        return;
    }
    int i = 0;
    while(js->pos < js->len) {
        type = jsonparse_next(js);
        if(type == JSON_TYPE_PAIR_NAME) {
            if(jsonparse_strcmp_value(js, "amount") == 0) {
                type = jsonparse_next(js);
                if(type != JSON_TYPE_PAIR) {
                    os_printf("Not a valid pour list 2: %x\n", (int)type);
                    return;
                }
                type = jsonparse_next(js);
                if(type != JSON_TYPE_STRING) {
                    os_printf("Not a valid  pour list 3: %x\n", (int)type);
                    return;
                }
                jsonparse_copy_value(js, buf, jsonparse_get_len(js)+1);
                int amount = atoi(buf);
                pour[i].amount = amount;
            }
            if(jsonparse_strcmp_value(js, "sequence") == 0) {
                type = jsonparse_next(js);
                if(type != JSON_TYPE_PAIR) {
                    os_printf("Not a valid pour list 4: %x\n", (int)type);
                    return;
                }
                type = jsonparse_next(js);
                if(type != JSON_TYPE_STRING) {
                    os_printf("Not a valid  pour list 5: %x\n", (int)type);
                    return;
                }
                jsonparse_copy_value(js, buf, jsonparse_get_len(js)+1);
                int sequence = atoi(buf);
                pour[i].sequence = sequence;
                i++;
            }
        }
    }
    // now that we parsed all that JSON bullshit, now we can finaly
    // pour the damn drink
    os_printf("json object parsed\n");
    pourexec();
}

// ----------------------------------------------------------------------------
static void client_http_callback(char *response, int http_status, char *full_response)
{
    if(http_status == 200) {
        os_printf("Response received\n");
        char *json = (char *)os_strstr(response, "{\"pour\":"); // find first occurence of...
        char *eol = os_strchr(json, 0x0d);  *eol = 0;            // Pointer to end of json.
        parsepour(json, strlen(json));
    }
    else {
        os_printf("http_status = %d\r\n", http_status);
        os_printf("strlen of response = %d\r\n", strlen(response));
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

    os_printf("HTTP Post\n");
    msgnum++;
    wifi_get_ip_info(STATION_IF, &ipConfig);
    os_sprintf(url,"%s", DATA_SERVER);       // Start the connection process
    os_printf("msg = %d\n", msgnum);
    os_printf("Request: %s\n%s\n", url, data);
    http_post(url, data, client_http_callback);
    os_printf("HTTP Post Completed\r\n");

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
    ioLed(1);                             // Turn test LED on
    wifi_check_ip();                       // Test for IP and connected
    if(connState == WIFI_CONNECTED) {
//      get_contents(&buffer[5]);          // get contetents
//      data_post(buffer);
        client_post(cont_jsn);
    }
    else {
        os_printf("WiFi not connected...\r\n");
    }
    ioLed(0);                    // Turn test LED off

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

