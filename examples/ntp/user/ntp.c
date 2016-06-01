// ----------------------------------------------------------------------------
// Simple NTP client for ESP8266.
// list of major public servers http://tf.nist.gov/tf-cgi/servers.cgi
// ----------------------------------------------------------------------------
#include <c_types.h>
#include <user_interface.h>
#include <espconn.h>
#include <osapi.h>
#include <mem.h>
#include <time.h>
#include "espmissingincludes.h"
#include "ntp.h"

// ----------------------------------------------------------------------------
// Global variables                                                                  
// ----------------------------------------------------------------------------
static uint8 ntp_server[] = {131, 107, 13, 100}; // microsoft NTP server
static os_timer_t ntp_timeout;
static struct espconn *pCon = 0;
char timestr[32];     // result gets put here

// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR ntp_udp_timeout(void *arg)
{
    os_timer_disarm(&ntp_timeout);
    os_sprintf(timestr,"NTP Timeout");
    os_printf("NTP Timeout\n");
    if(pCon) {                                // clean up connection
        espconn_delete(pCon);
        os_free(pCon->proto.udp);
        os_free(pCon);
        pCon = 0;
    }
}

// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR ntp_udp_recv(void *arg, char *pdata, unsigned short len)
{
    struct tm *dt;
    time_t timestamp;
    ntp_t *ntp;

    os_timer_disarm(&ntp_timeout);
    os_printf("received NTP message:\n");
    ntp = (ntp_t*)pdata;      // extract ntp time
    timestamp = ntp->trans_time[0] << 24 | ntp->trans_time[1] << 16 |ntp->trans_time[2] << 8 | ntp->trans_time[3];
    timestamp -= 2208988800UL;        // convert to unix time
    dt = gmtime(&timestamp);          // create tm struct
    os_sprintf(timestr,"%02d/%02d/%02d %02d:%02d:%02d", dt->tm_mon+1,dt->tm_mday,dt->tm_year+1900,dt->tm_hour,dt->tm_min,dt->tm_sec);
    os_printf("%s\n",timestr);
    if(pCon) {                                // clean up connection
        espconn_delete(pCon);
        os_free(pCon->proto.udp);
        os_free(pCon);                                                                           
        pCon = 0;
    }
}

// ----------------------------------------------------------------------------
// send the NTP request
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR ntp_get_time(void)
{
    ntp_t ntp;

    os_printf("Getting NTP:\n");
    pCon = (struct espconn*)os_zalloc(sizeof(struct espconn));
    pCon->type = ESPCONN_UDP;
    pCon->state = ESPCONN_NONE;
    pCon->proto.udp = (esp_udp*)os_zalloc(sizeof(esp_udp));
    pCon->proto.udp->local_port  = espconn_port();
    pCon->proto.udp->remote_port = 123;
    os_memcpy(pCon->proto.udp->remote_ip, ntp_server, 4);
    espconn_regist_recvcb(pCon, ntp_udp_recv);
    espconn_create(pCon);                      // send the ntp request

    os_timer_disarm(&ntp_timeout);      // set timeout timer
    os_timer_setfn(&ntp_timeout, (os_timer_func_t*)ntp_udp_timeout, pCon);
    os_timer_arm(&ntp_timeout, NTP_TIMEOUT_MS, 0);

    os_memset(&ntp, 0, sizeof(ntp_t));  // create a really simple ntp request packet
    ntp.options = 0b00100011;           // leap = 0, version = 4, mode = 3 (client)
    espconn_sent(pCon, (uint8*)&ntp, sizeof(ntp_t));
}
// ----------------------------------------------------------------------------

