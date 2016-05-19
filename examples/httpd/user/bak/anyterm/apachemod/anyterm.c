// apachemod/anyterm.c
// This file is part of Anyterm; see http://chezphil.org/anyterm/
// (C) 2005 Philip Endecott

// This is version $Name$
//   (if there is no version (e.g. V0-1) mentioned in the previous line,
//    this is probably a snapshot from between "official" releases.)

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.


#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "http_request.h"
#include "util_script.h"
#include "http_connection.h"
#include "apr_strings.h"

#include <pty.h> 
#include <unistd.h>



module AP_MODULE_DECLARE_DATA anyterm;


static int master_fd;


static void report_error(request_rec* r, char* what)
{
  ap_rputs("<error>",r);
  ap_rputs(strerror(errno),r);
  ap_rputs(" when ",r);
  ap_rputs(what,r);
  ap_rputs("</error>",r);
}


static void anyterm_open(request_rec* r)
{
  pid_t pid=forkpty(&master_fd, NULL, NULL, NULL);
  if (pid==-1) {
    report_error(r,"forkpty()");
    return;
  }
  if (pid==0) {
    //execl("/bin/login","login",NULL);
    execl("/bin/bash","bash",NULL);
    report_error(r,"execl() returned");
    return;
  }
  int rc=fcntl(master_fd,F_SETFL,O_NONBLOCK);
  if (rc==-1) {
    report_error(r,"fcntl(F_SETFL,O_NONBLOCK)");
    return;
  }
  ap_rputs("<open/>",r);
}

static void anyterm_close(request_rec* r)
{
  int rc=close(master_fd);
  if (rc==-1) {
    report_error(r,"close()");
    return;
  }
  ap_rputs("<close/>",r);
}



static void anyterm_input(request_rec* r, const char* k)
{
  int l=strlen(k);
  if (l>0) {
    char* dk=malloc(l+1);
    if (!dk) {
      report_error(r,"malloc()");
    }
    const char* p=k;
    char* q=dk;
    char hexbuf[3];
    while(*p) {
      if (*p=='+') {
	*q=' ';
	p++;
      } else if (*p=='%') {
	hexbuf[0]=p[1];
	hexbuf[1]=p[2];
	hexbuf[2]='\0';
	*q=strtol(hexbuf,NULL,16);
	p+=3;
      } else {
	*q=*p;
	p++;
      }
      q++;
    }
    int dl=q-dk;
    int rc=write(master_fd,dk,dl);
    free(dk);
    if (rc!=dl) {
      report_error(r,"write()");
      return;
    }
  }
  int first=1;
  static char buf[256];
  int n;
  while(1) {
    n=read(master_fd,buf,sizeof(buf)-1);
    if (n==-1) {
      if (errno==EAGAIN) {
	n=0;
      } else {
	report_error(r,"read()");
	if (first) {
	  return;
	} else {
	  break;
	}
      }
    }
    if (first) {
      ap_rputs("<op>",r);
      first=0;
    }
    if (n==0) {
      break;
    }
    buf[n]='\0';
    ap_rputs(buf,r);
  }
  ap_rputs("</op>",r);
}


static int anyterm_handler(request_rec* r)
{
  if (strcmp(r->handler,"anyterm")) {
    return DECLINED;
  }

  ap_set_content_type(r,"text/xml");
  if (r->header_only) {
    return OK;
  }

  char* args;
  if (r->args) {
    args=r->args;
  } else {
    args="";
  }

  ap_rputs("<?xml version=\"1.0\"?>\n",r);

  if (strcmp(args,"open")==0) {
    anyterm_open(r);
  } else if (strcmp(args,"close")==0) {
    anyterm_close(r);
  } else if (strncmp(args,"k=",2)==0) {
    const char* k=args+2;
    anyterm_input(r,k);
  } else {
    ap_rputs("<error>invalid args '",r);
    ap_rputs(args,r);
    ap_rputs("'</error>",r);
  }
  
  return OK;
}


static void anyterm_register_hooks(apr_pool_t *p)
{
  ap_hook_handler(anyterm_handler, NULL, NULL, APR_HOOK_MIDDLE);
}



module AP_MODULE_DECLARE_DATA anyterm = {
  STANDARD20_MODULE_STUFF,
  NULL, /* per-directory config creator */
  NULL, /* dir config merger */
  NULL, /* server config creator */
  NULL, /* server config merger */
  NULL, /* command table */
  anyterm_register_hooks,  /* set up other request processing hooks */
};

