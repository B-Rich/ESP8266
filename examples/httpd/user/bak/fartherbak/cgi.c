// ----------------------------------------------------------------------------
// CGI routines. Used in the LED example and the page that returns
// the entire flash as a binary. Also handles the hit counter on the main page.
// ----------------------------------------------------------------------------
#include <string.h>
#include <osapi.h>
#include <ip_addr.h>
#include "espmissingincludes.h"
#include "user_interface.h"
#include "espfs.h"
#include "mem.h"
#include "httpd.h"
#include "cgi.h"
#include "io.h"
#include "stdout.h"

static long hitCounter   = 0;     // web page hit counter
static char currLedState = 0;     // Current state of the LED
static int  drinknum     = 0;     // Drink index number

// ----------------------------------------------------------------------------
// CGI that turns the LED on or off according to the 'led' param in the POST data
// ----------------------------------------------------------------------------
int ICACHE_FLASH_ATTR cgiMixDrink(HttpdConnData *connData)
{
    int len;
    char buff[1024];

    if(connData->conn == NULL) {    // Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    len = httpdFindArg(connData->postBuff, "drink", buff, sizeof(buff));
    if(len != 0) {
        drinknum = atoi(buff);
        os_printf("\n\rDrink num = %d\n\r",drinknum);  // drink number

//      ioMux(1);               // switch to PIC RS232 Port
        
        stdoutUartTxd('>');    // send command to PIC
        stdoutUartTxd('T');
        stdoutUartTxd('1');
        stdoutUartTxd('0');
        stdoutUartTxd('0');
        stdoutUartTxd(0xd);
        stdoutUartTxd(0x0);

//      ioMux(0);               // switch to Debug RS232 Port
    }
    httpdRedirect(connData, "making.htm");
    return HTTPD_CGI_DONE;
}

// ----------------------------------------------------------------------------
// Template code for the Mixing page.
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR tplMixing(HttpdConnData *connData, char *token, void **arg)
{
    char buff[128];
    if(token == NULL) return;

    if(os_strcmp(token, "drinknum") == 0) {
        os_sprintf(buff, "%d", drinknum);
    }
    httpdSend(connData, buff, -1);
}

// ----------------------------------------------------------------------------
// CGI that turns the LED on or off according to the 'led' param in the POST data
// ----------------------------------------------------------------------------
int ICACHE_FLASH_ATTR cgiLed(HttpdConnData *connData)
{
    int len;
    char buff[1024];

    if(connData->conn == NULL) {    // Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    len = httpdFindArg(connData->postBuff, "led", buff, sizeof(buff));
    if(len != 0) {
        currLedState = atoi(buff);
        ioLed(currLedState);
        ioMux(currLedState);               // switch to PIC RS232 Port        
    }

    httpdRedirect(connData, "led.htm");
    return HTTPD_CGI_DONE;
}

// ----------------------------------------------------------------------------
// Template code for the led page.
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR tplLed(HttpdConnData *connData, char *token, void **arg)
{
    char buff[128];
    if(token == NULL) return;

    os_strcpy(buff, "Unknown");
    if(os_strcmp(token, "ledstate") == 0) {
        if(currLedState) {
            os_strcpy(buff, "on");
        }
        else {
            os_strcpy(buff, "off");
        }
    }
    httpdSend(connData, buff, -1);
}

// ----------------------------------------------------------------------------
// Template code for the counter on the index page.
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR tplCounter(HttpdConnData *connData, char *token, void **arg)
{
    char buff[128];
    if(token==NULL) return;

    if(os_strcmp(token, "counter") == 0) {
        hitCounter++;
        os_sprintf(buff, "%ld", hitCounter);
    }
    httpdSend(connData, buff, -1);
}

// ----------------------------------------------------------------------------
// CGI that reads the json db and returns it
// ----------------------------------------------------------------------------
int ICACHE_FLASH_ATTR cgiReadFlash(HttpdConnData *connData)
{
    EspFsFile *file = connData->cgiData;
    int len;
    char buff[1024];
    
    if(connData->conn == NULL) {           //Connection aborted. Clean up.
        espFsClose(file);
        return HTTPD_CGI_DONE;
    }

    if(file == NULL) {    // First call to this cgi. Open the file so we can read it.
        file = espFsOpen("cocktails.jsn");
        if(file == NULL) {
            return HTTPD_CGI_NOTFOUND;
        }
        connData->cgiData = file;
        httpdStartResponse(connData, 200);
        httpdHeader(connData, "Content-Type", httpdGetMimetype(connData->url));
        httpdHeader(connData, "Cache-Control", "max-age=3600, must-revalidate");
        httpdEndHeaders(connData);
        return HTTPD_CGI_MORE;
    }

    len = espFsRead(file, buff, 1024);
    if(len > 0) espconn_sent(connData->conn, (uint8 *)buff, len);
    if(len != 1024) {
        espFsClose(file);             // done.
        return HTTPD_CGI_DONE;
    } 
    else {
        return HTTPD_CGI_MORE;        // Ok, till next time.
    }
}

// ----------------------------------------------------------------------------

