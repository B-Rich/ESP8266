// ----------------------------------------------------------------------------
// CGI routines
// ----------------------------------------------------------------------------
#ifndef CGI_H
#define CGI_H

#include "httpd.h"

int cgiloadRBF(HttpdConnData *connData);
int cgiRBFlash(HttpdConnData *connData);

int  cgiLed(HttpdConnData *connData);
void tplLed(HttpdConnData *connData, char *token, void **arg);

int  cgiReadFlash(HttpdConnData *connData);

void tplCounter(HttpdConnData *connData, char *token, void **arg);

int  cgiGetContents(HttpdConnData *connData);
int  cgiPutContents(HttpdConnData *connData);


#endif
// ----------------------------------------------------------------------------

