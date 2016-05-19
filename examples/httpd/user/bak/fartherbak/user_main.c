// ----------------------------------------------------------------------------
// Main Function
// ----------------------------------------------------------------------------
#include "espmissingincludes.h"
#include "ets_sys.h"
#include "osapi.h"
#include "httpd.h"
#include "io.h"
#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
#include "stdout.h"
#include "auth.h"

// ----------------------------------------------------------------------------
// Function that tells the authentication system what users/passwords live on the system.
// This is disabled in the default build; if you want to try it, enable the authBasic line in
// the builtInUrls below.
// ----------------------------------------------------------------------------
int myPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen)
{
    if(no == 0) {
        os_strcpy(user, "admin");
        os_strcpy(pass, "admin");
        return 1;
    }                   
    else if(no == 1) {
        os_strcpy(user, "user1");
        os_strcpy(pass, "password");
        return 1;
    }
    // Add more here . Check against incrementing 'no' for each user added.
    return 0;
}

// ----------------------------------------------------------------------------
// This is the main url->function dispatching data struct.
// In short, it's a struct with various URLs plus their handlers. The handlers 
// can be 'standard' CGI functions you wrote, or 'special' CGIs requiring an 
// argument. They can also be auth-functions. An asterisk will match any url 
// starting with everything before the asterisks; "*" matches everything. The
// list will be handled top-down, so make sure to put more specific rules above
// the more general ones. Authorization things (like authBasic) act as a 
// 'barrier' and should be placed above the URLs they protect.
// ----------------------------------------------------------------------------
HttpdBuiltInUrl builtInUrls[] = {
    {"/",           cgiRedirect, "/index.htm"},
    {"/index.htm",  cgiEspFsTemplate, tplCounter},
    {"/led.htm",    cgiEspFsTemplate, tplLed},
    {"/making.htm", cgiEspFsTemplate, tplMixing},

    {"/dbfile",     cgiReadFlash, NULL},
    {"/led.cgi",    cgiLed,       NULL},
    {"/drink.cgi",  cgiMixDrink,  NULL},

// Routines to make the /wifi URL and everything beneath it work.
// Enable the line below to protect the WiFi configuration with an username/password combo.
//  {"/wifi/*", authBasic, myPassFn},

    {"/wifi",              cgiRedirect, "/wifi/wifi.htm"},
    {"/wifi/",             cgiRedirect, "/wifi/wifi.htm"},
    {"/wifi/wifiscan.cgi", cgiWiFiScan, NULL},
    {"/wifi/wifi.htm",     cgiEspFsTemplate, tplWlan},
    {"/wifi/connect.cgi",  cgiWiFiConnect, NULL},
    {"/wifi/setmode.cgi",  cgiWifiSetMode, NULL},

    {"*", cgiEspFsHook, NULL}, // Catch-all cgi function for the filesystem
    {NULL, NULL, NULL}
};

// ----------------------------------------------------------------------------
// Main routine. Initialize stdout, the I/O and the webserver and we're done.
// ----------------------------------------------------------------------------
void user_init(void)
{
    stdoutInit();
    ioInit();
    httpdInit(builtInUrls, 80);
    os_printf("\nReady\n");
}

// ----------------------------------------------------------------------------

