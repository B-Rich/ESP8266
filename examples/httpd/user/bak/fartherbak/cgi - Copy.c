// ----------------------------------------------------------------------------
// CGI routines. Used in the LED example and the page that returns
// the entire flash as a binary. Also handles the hit counter on the main page.
// ----------------------------------------------------------------------------
#include <ets_sys.h>
#include <string.h>
#include <osapi.h>
#include <ip_addr.h>
#include <mem.h>
#include "espmissingincludes.h"
#include "user_interface.h"
#include "stdout.h"
#include "espfs.h"
#include "httpd.h"
#include "cgi.h"
#include "io.h"
#include "base64.h"
#include "contents.h"

// ----------------------------------------------------------------------------
// Global definitions and variables
// ----------------------------------------------------------------------------
#define CONSOLEPORT       0    // Port that the debug console is connected to

static long hitCounter  = 0;   // web page hit counter
static int  currState   = 0;   // Current state

// ----------------------------------------------------------------------------
#define FLASHCONTENTS     0x80000               // Flash RAM location for user memory
#define RBFRECORDS        167                   // each record is 1024 bytes 170,308

// ----------------------------------------------------------------------------
// CGI that reads the json db and returns it    
// ----------------------------------------------------------------------------
extern void wdt_feed(void);
extern void ets_wdt_disable(void);
extern void ets_wdt_enable(void);

int ICACHE_FLASH_ATTR RBFfromFlash(void)
{
    uint8 buf[1024];             // place for the data
    uint32 flash_addr = FLASHCONTENTS;  //  user memory
    int len = sizeof(buf);

    os_printf("loading RBF from flash:\n");
    ets_wdt_disable();

    SetnConfig(0);
    os_delay_us(1000);   // 1ms delay
    SetnConfig(1);
    for(int i = 0; i < RBFRECORDS; i++) {
        SpiFlashOpResult ret = spi_flash_read(flash_addr, (uint32 *)buf, len);
        if(ret != SPI_FLASH_RESULT_OK) {
            os_printf("flash read error: %d\r\n", (int)ret);
            break;
        }
        for(int j = 0; j < len; j++) {
            LoadFPGAByte(buf[j]);
        }
        flash_addr += len;
    }
    ets_wdt_enable();
    os_printf("done");
    return(HTTPD_CGI_DONE);
}

// ----------------------------------------------------------------------------
// Load RBF file from client and upload to the FPGA
// ----------------------------------------------------------------------------
#define B64IBUFSIZE 1024
#define B64OBUFSIZE 4*(B64IBUFSIZE/3)+4
int ICACHE_FLASH_ATTR cgiloadRBF(HttpdConnData *connData)
{
    static int chunk = 0;
    int ilen;
    char ibuf[B64IBUFSIZE];
    unsigned char obuf[B64OBUFSIZE];

    if(connData->conn == NULL) {    // Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }
    ilen = httpdFindArg(connData->postBuff, "RBF", ibuf, sizeof(ibuf));
    if(ilen != 0) {
        if(ibuf[0] == 'S') {                      // fist chunk
            os_printf("Starting RBF...\n");
            SetnConfig(0);
            os_delay_us(1000);                    // 1us delay
            SetnConfig(1);
        }
        else if(ibuf[0] == 'E') {     // last one
            chunk = 0;
            os_printf("Done loading RBF...\n");
        }
        else if(ibuf[0] == 'B') {
            os_printf("chunk#: %d\n", chunk);
            int olen = base64_decode(ilen-1, &ibuf[1], B64OBUFSIZE, obuf);
            for(int j = 0; j < olen; j++) {
                LoadFPGAByte(obuf[j]);
            }
            chunk++;
        }
        else if(ibuf[0] == 'F') {
            RBFfromFlash();
        }
        else os_printf("something went wrong\n");
        ibuf[21] = 0; os_printf("ibuf[20]=[%s]\n", ibuf);
    }
    httpdRedirect(connData, "index.htm");
    return(HTTPD_CGI_DONE);
}

// ----------------------------------------------------------------------------
// CGI that turns the LED on or off according to the 'led' param in the POST data
// ----------------------------------------------------------------------------
int ICACHE_FLASH_ATTR cgiLed(HttpdConnData *connData)
{
    int  len;
    char buff[1024];

    if(connData->conn == NULL) {    // Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    len = httpdFindArg(connData->postBuff, "led", buff, sizeof(buff));
    if(len != 0) {
        currState = atoi(buff);
        ioLed(currState);
        ioMux(currState);               // switch to PIC RS232 Port
    }
    httpdRedirect(connData, "test.htm");
    return(HTTPD_CGI_DONE);
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
        if(currState) os_strcpy(buff, "on");
        else          os_strcpy(buff, "off");
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

    if(connData->conn == NULL) {           // Connection aborted. Clean up.
        espFsClose(file);
        return(HTTPD_CGI_DONE);
    }
    if(file == NULL) {    // First call to this cgi. Open the file so we can read it.
        file = espFsOpen("cocktails.jsn");
        if(file == NULL) {
            return(HTTPD_CGI_NOTFOUND);
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
        return(HTTPD_CGI_DONE);
    }
    else {
        return(HTTPD_CGI_MORE);        // Ok, till next time.
    }
}

// ----------------------------------------------------------------------------
// CGI that reads the json db and returns it
// ----------------------------------------------------------------------------
int ICACHE_FLASH_ATTR cgiGetContents(HttpdConnData *connData)
{
    int len;
    char buffer[1024];

    httpdStartResponse(connData, 200);
    httpdHeader(connData, "Content-Type","text/json");
    httpdHeader(connData, "Access-Control-Allow-Origin","*"); // for firefox assholes
    httpdHeader(connData, "Cache-Control", "max-age=60, must-revalidate");
    httpdEndHeaders(connData);
    len = get_contents(buffer);
    if(len > 0) httpdSend(connData, buffer, len);
    else        return(HTTPD_CGI_NOTFOUND);
    return(HTTPD_CGI_DONE);
}

// ----------------------------------------------------------------------------
// CGI that receives the json list of canister contents
// ----------------------------------------------------------------------------
int ICACHE_FLASH_ATTR cgiPutContents(HttpdConnData *connData)
{
    int len;
    if(connData->conn == NULL) {    // Connection aborted. Clean up.
        return(HTTPD_CGI_DONE);
    }
    len = strlen(connData->postBuff);
    if(len != 0) {     // save it in Flash Ram
        put_contents(connData->postBuff, len);
    }
    return(HTTPD_CGI_DONE);
}

// ----------------------------------------------------------------------------

