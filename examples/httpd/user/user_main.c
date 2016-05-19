// ----------------------------------------------------------------------------
// Main Function
// ----------------------------------------------------------------------------
#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <user_interface.h>
#include "espmissingincludes.h"
#include "io.h"
#include "httpd.h"
#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
#include "stdout.h"
#include "auth.h"
#include "client.h"

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
        return(1);
    }
    else if(no == 1) {
        os_strcpy(user, "user1");
        os_strcpy(pass, "password");                                                                
        return(1);
    }
    return(0); // Add more here . Check against incrementing 'no' for each user added.
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
    { "/",                  cgiRedirect,      "/index.htm"       },
    { "/index.htm",         cgiEspFsTemplate,  tplCounter        },
    { "/test.htm",          cgiEspFsTemplate,  tplLed            },

    { "/dbfile",            cgiReadFlash,      NULL              },
    { "/contents.cgi",      cgiGetContents,    NULL              },
    { "/save.cgi",          cgiPutContents,    NULL              },

    { "/led.cgi",           cgiLed,            NULL              },
//  { "/testSPI.cgi",       testSPI,           NULL              },
    { "/loadrbf.cgi",       cgiloadRBF,        NULL              },
    { "/rbflash.cgi",       cgiRBFlash,        NULL              },

    { "/wifi",              cgiRedirect,       "/wifi/wifi.htm"  },
    { "/wifi/",             cgiRedirect,       "/wifi/wifi.htm"  },
    { "/wifi/wifiscan.cgi", cgiWiFiScan,       NULL              },
    { "/wifi/wifi.htm",     cgiEspFsTemplate,  tplWlan           },
    { "/wifi/connect.cgi",  cgiWiFiConnect,    NULL              },
    { "/wifi/setmode.cgi",  cgiWifiSetMode,    NULL              },                                                      
//  { "/wifi/*",            authBasic,         myPassFn          },

    {"*",                   cgiEspFsHook,     NULL               }, // Catch-all
    { NULL,                 NULL,             NULL               }  // end of table
};

// ----------------------------------------------------------------------------
// Change my SSID
// ----------------------------------------------------------------------------
static void user_SSID(char *ssid)
{
    struct softap_config ap_cfg;
    while(!wifi_softap_get_config(&ap_cfg));
    memset(ap_cfg.ssid, 0, 32);
    os_memcpy(&ap_cfg.ssid, ssid, strlen(ssid));
    wifi_softap_set_config(&ap_cfg);
}

// ----------------------------------------------------------------------------
// Main routine. Initialize stdout, the I/O and the webserver and we're done.
// ----------------------------------------------------------------------------
void user_init(void)
{
    int i;
    stdoutInit();
    ioInit();               // initialize UART, GPIO and crap like that
    for(i = 0; i < 30; i++) os_printf(" \r\n");
    os_printf("HTTPD Program.\n\r");
    user_SSID("TenForward");
    httpdInit(builtInUrls, 80);

    os_printf("\n\rGuinan is Ready\n\r");
}

// ----------------------------------------------------------------------------

