// ----------------------------------------------------------------------------
// CGI routines
// ----------------------------------------------------------------------------
#ifndef CGI_H
#define CGI_H

#include "httpd.h"

void tplMixing(HttpdConnData *connData, char *token, void **arg);
int  cgiMixDrink(HttpdConnData *connData);
int  cgiLed(HttpdConnData *connData);
void tplLed(HttpdConnData *connData, char *token, void **arg);
int  cgiReadFlash(HttpdConnData *connData);
void tplCounter(HttpdConnData *connData, char *token, void **arg);

#endif
// ----------------------------------------------------------------------------

