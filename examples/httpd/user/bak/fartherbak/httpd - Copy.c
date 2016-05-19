// ----------------------------------------------------------------------------
// Esp8266 http server - core routines
// ----------------------------------------------------------------------------
#include <ets_sys.h>
#include <c_types.h>
#include <mem.h>
#include <osapi.h>
#include <user_interface.h>
#include <espconn.h>

#include "espmissingincludes.h"
#include "httpd.h"
#include "io.h"
#include "espfs.h"

#include "user_config.h"

// ----------------------------------------------------------------------------
// Global constants and variables
// ----------------------------------------------------------------------------
#define MAX_HEAD_LEN     1024  // Max length of request head
#define MAX_CONN         8     // Max amount of connections
#define MAX_POST         1024  // Max post buffer len
#define MAX_SENDBUFF_LEN 2048  // Max send buffer len

static HttpdBuiltInUrl *builtInUrls; // This gets set at init time.

// ----------------------------------------------------------------------------
// Private data for http connection
// ----------------------------------------------------------------------------
struct HttpdPriv {
    char head[MAX_HEAD_LEN];
    int headPos;
    int postPos;
    char *sendBuff;
    int sendBuffLen;
};

// ----------------------------------------------------------------------------
// Connection pool
// ----------------------------------------------------------------------------
static HttpdPriv connPrivData[MAX_CONN];
static HttpdConnData connData[MAX_CONN];

// ----------------------------------------------------------------------------
// Listening connection data
// ----------------------------------------------------------------------------
static struct espconn httpdConn;
static esp_tcp httpdTcp;

// ----------------------------------------------------------------------------
//Struct to keep extension->mime data in
// ----------------------------------------------------------------------------
typedef struct {
    const char *ext;
    const char *mimetype;
} MimeMap;

// ----------------------------------------------------------------------------
// The mappings from file extensions to mime types.
// If you need an extra mime type, add it here.
// ----------------------------------------------------------------------------
static const MimeMap mimeTypes[]={
//  {"htm",  "text/htm"       }, // do not process as mime
    {"html", "text/html"      },
    {"css",  "text/css"       },
    {"js",   "text/javascript"},
    {"txt",  "text/plain"     },
    {"jpg",  "image/jpeg"     },
    {"jpeg", "image/jpeg"     },
    {"png",  "image/png"      },
    {NULL,   "text/html"      }, // default value
};

// ----------------------------------------------------------------------------
// Returns a static char* to a mime type for a given url to a file.
// ----------------------------------------------------------------------------
const char ICACHE_FLASH_ATTR *httpdGetMimetype(char *url)
{
    int i=0;

    char *ext=url+(strlen(url)-1);
    while(ext!=url && *ext!='.') ext--;        // Go find the extension
    if(*ext == '.') ext++;
   
    // ToDo: os_strcmp is case sensitive; we may want to do case-intensive matching here...
    while(mimeTypes[i].ext!=NULL && os_strcmp(ext, mimeTypes[i].ext)!=0) i++;
    return(mimeTypes[i].mimetype);
}

// ----------------------------------------------------------------------------
// Looks up the connData info for a specific esp connection
// ----------------------------------------------------------------------------
static HttpdConnData ICACHE_FLASH_ATTR *httpdFindConnData(void *arg)
{
    for(int i = 0; i < MAX_CONN; i++) {
        if(connData[i].conn == (struct espconn *)arg) return &connData[i];
    }
    #if DEBUG_HTTPD_MSG
        os_printf("FindConnData: Warning, Couldn't find connection for %p\n", arg);
    #endif
    return(NULL);
}

// ----------------------------------------------------------------------------
// Retires a connection for re-use
// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR httpdRetireConn(HttpdConnData *conn)
{
    if(conn->postBuff != NULL) os_free(conn->postBuff);
    conn->postBuff = NULL;
    conn->cgi      = NULL;
    conn->conn     = NULL;
}

// ----------------------------------------------------------------------------
// helper function that returns the value of a hex char.
// ----------------------------------------------------------------------------
static int httpdHexVal(char c)
{
    if(c >= '0' && c <= '9') return c-'0';
    if(c >= 'A' && c <= 'F') return c-'A'+10;
    if(c >= 'a' && c <= 'f') return c-'a'+10;
    return 0;
}

// ----------------------------------------------------------------------------
// Decode a percent-encoded value.
// Takes the valLen bytes stored in val, and converts it into at most retLen bytes that
// are stored in the ret buffer. Returns the actual amount of bytes used in ret. Also
// zero-terminates the ret buffer.
// ----------------------------------------------------------------------------
int httpdUrlDecode(char *val, int valLen, char *ret, int retLen)
{
    int s = 0, d = 0;
    int esced = 0, escVal = 0;
    while(s < valLen && d < retLen) {
        if(esced == 1)  {
            escVal = httpdHexVal(val[s])<<4;
            esced = 2;
        }
        else if(esced == 2) {
            escVal += httpdHexVal(val[s]);
            ret[d++] = escVal;
            esced = 0;
        }
        else if(val[s] == '%') {
            esced = 1;
        }
        else if(val[s] == '+') {
            ret[d++] = ' ';
        }
        else {
            ret[d++] = val[s];
        }
        s++;
    }
    if(d < retLen) ret[d] = 0;
    return d;
}

// ----------------------------------------------------------------------------
// Find a specific arg in a string of get- or post-data.
// Line is the string of post/get-data, arg is the name of the value to find. The
// zero-terminated result is written in buff, with at most buffLen bytes used. The
// function returns the length of the result, or -1 if the value wasn't found. The
// returned string will be urldecoded already.
// ----------------------------------------------------------------------------
int ICACHE_FLASH_ATTR httpdFindArg(char *line, char *arg, char *buff, int buffLen)
{
    char *p, *e;
    if(line == NULL) return 0;
    p = line;
    while(p != NULL && *p != '\n' && *p != '\r' && *p != 0) {
        #if DEBUG_HTTPD
            os_printf("findArg: %s\n", p);
        #endif
        if(os_strncmp(p, arg, os_strlen(arg))==0 && p[strlen(arg)] == '=') {
            p += os_strlen(arg)+1; //move p to start of value
            e = (char*)os_strstr(p, "&");
            if(e == NULL) e = p + os_strlen(p);
            #if DEBUG_HTTPD
                os_printf("findArg: val %s len %d\n", p, (e-p));
            #endif
            return httpdUrlDecode(p, (e-p), buff, buffLen);
        }
        p = (char*)os_strstr(p, "&");
        if(p != NULL) p += 1;
    }
    #if DEBUG_HTTPD_MSG
        os_printf("Finding %s in %s: Not found: /\n", arg, line);
    #endif
    return(-1);  // not found
}

// ----------------------------------------------------------------------------
// Get the value of a certain header in the HTTP client head
// ----------------------------------------------------------------------------
int ICACHE_FLASH_ATTR httpdGetHeader(HttpdConnData *conn, char *header, char *ret, int retLen)
{
    char *p=conn->priv->head;
    p = p + strlen(p) + 1; // skip GET/POST part
    p = p + strlen(p) + 1; // skip HTTP part
    while(p < (conn->priv->head+conn->priv->headPos)) {
        while(*p <= 32 && *p!=0) p++; //skip crap at start
        #if DEBUG_HTTPD
            os_printf("Looking for %s, Header: '%s'\n", header, p);
        #endif
        // See if this is the header
        if(os_strncmp(p, header, strlen(header)) == 0 && p[strlen(header)] == ':') {
            p = p + strlen(header)+1;                // Skip 'key:' bit of header line
            while(*p == ' ') p++;                    // Skip past spaces after the colon
            while(*p != 0 && *p != '\r' && *p != '\n' && retLen > 1) {   // Copy from p to end
                *ret++ = *p++;
                retLen--;
            }
            *ret = 0;   // Zero-terminate string
            return 1;   // done
        }
        p += strlen(p) +1;     // Skip past end of string and 0 terminator
    }
    return(0);
}

// ----------------------------------------------------------------------------
// Start the response headers.
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR httpdStartResponse(HttpdConnData *conn, int code)
{
    char buff[128];
    int l;
    l = os_sprintf(buff, "HTTP/1.0 %d OK\r\nServer: ypu-httpd/ %s\r\n", code, HTTPDVER);
    httpdSend(conn, buff, l);
}

// ----------------------------------------------------------------------------
// Send a http header.
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR httpdHeader(HttpdConnData *conn, const char *field, const char *val)
{
    char buff[256];
    int l;
    l = os_sprintf(buff, "%s: %s\r\n", field, val);
    httpdSend(conn, buff, l);
}

// ----------------------------------------------------------------------------
// Finish the headers.
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR httpdEndHeaders(HttpdConnData *conn)
{
    httpdSend(conn, "\r\n", -1);
}

// ----------------------------------------------------------------------------
// ToDo: sprintf->snprintf everywhere... esp doesn't have snprintf tho' :/
// Redirect to the given URL.
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR httpdRedirect(HttpdConnData *conn, char *newUrl)
{
    char buff[1024];
    int l;
    l = os_sprintf(buff,"HTTP/1.1 302 Found\r\nLocation: %s \r\n\r\nMoved to %s \r\n", newUrl, newUrl);
    httpdSend(conn, buff, l);
}

// ----------------------------------------------------------------------------
// Use this as a cgi function to redirect one url to another.
// ----------------------------------------------------------------------------
int ICACHE_FLASH_ATTR cgiRedirect(HttpdConnData *connData)
{
    if(connData->conn == NULL) {
        return HTTPD_CGI_DONE;           // Connection aborted. Clean up.
    }
    httpdRedirect(connData, (char*)connData->cgiArg);
    return(HTTPD_CGI_DONE);
}

// ----------------------------------------------------------------------------
// Add data to the send buffer. len is the length of the data. If len is -1
// the data is seen as a C-string.
// Returns 1 for success, 0 for out-of-memory.
// ----------------------------------------------------------------------------
int ICACHE_FLASH_ATTR httpdSend(HttpdConnData *conn, const char *data, int len)
{
    if(len < 0) len = strlen(data);
    if(conn->priv->sendBuffLen+len > MAX_SENDBUFF_LEN) return 0;
    os_memcpy(conn->priv->sendBuff + conn->priv->sendBuffLen, data, len);
    conn->priv->sendBuffLen += len;
    return(1);
}

// ----------------------------------------------------------------------------
// Helper function to send any data in conn->priv->sendBuff
// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR xmitSendBuff(HttpdConnData *conn)
{
    if(conn->priv->sendBuffLen != 0) {
        espconn_sent(conn->conn, (uint8_t*)conn->priv->sendBuff, conn->priv->sendBuffLen);
        conn->priv->sendBuffLen = 0;
    }
}

// ----------------------------------------------------------------------------
// Callback called when the data on a socket has been successfully sent.
// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR httpdSentCb(void *arg)
{
    int r;
    HttpdConnData *conn=httpdFindConnData(arg);
    char sendBuff[MAX_SENDBUFF_LEN];

    #if DEBUG_HTTPD
        os_printf("Sent callback on conn %p\n", conn);
    #endif
    if(conn == NULL) return;
    conn->priv->sendBuff = sendBuff;
    conn->priv->sendBuffLen = 0;
    if(conn->cgi == NULL) {     // Marked for destruction?
        os_printf("Conn %p is done. Closing.\n", conn->conn);
        espconn_disconnect(conn->conn);
        httpdRetireConn(conn);
        return;     // No need to call xmitSendBuff.
    }
    r = conn->cgi(conn);         // Execute cgi fn.
    if(r == HTTPD_CGI_DONE) {
        conn->cgi = NULL;         // mark for destruction.
    }
    xmitSendBuff(conn);
}

static const char *httpNotFoundHeader = {
    "HTTP/1.0 404 Not Found\r\n"
    "Server:ypu-httpd/0.1\r\n"
    "Content-Type: text/plain\r\n\r\n"
    "Not Found.\r\n"
};

// ----------------------------------------------------------------------------
// This is called when the headers have been received and the connection is ready to send
// the result headers and data.
// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR httpdSendResp(HttpdConnData *conn)
{
    int i=0;
    int r;

    // See if the url is somewhere in our internal url table.
    while(builtInUrls[i].url != NULL && conn->url != NULL) {
        int match = 0;
        #if DEBUG_HTTPD
            os_printf("%s == %s?\n", builtInUrls[i].url, conn->url);
        #endif
        if(os_strcmp(builtInUrls[i].url, conn->url) == 0) match = 1;
        if(builtInUrls[i].url[os_strlen(builtInUrls[i].url)-1] == '*' &&
                os_strncmp(builtInUrls[i].url, conn->url, os_strlen(builtInUrls[i].url)-1) == 0) match = 1;
        if(match) {
            os_printf("Is url index %d\n", i);
            conn->cgiData = NULL;
            conn->cgi = builtInUrls[i].cgiCb;
            conn->cgiArg = builtInUrls[i].cgiArg;
            r = conn->cgi(conn);
            if(r != HTTPD_CGI_NOTFOUND) {
                if(r == HTTPD_CGI_DONE) conn->cgi = NULL;  // If cgi finishes immediately: mark conn for destruction.
                return;
            }
        }
        i++;
    }
    os_printf("%s not found. 404!\n", conn->url);    // Can't find
    httpdSend(conn, httpNotFoundHeader, -1);
    conn->cgi = NULL;   // mark for destruction
}

// ----------------------------------------------------------------------------
// Parse a line of header data and modify the connection data accordingly.
// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR httpdParseHeader(char *h, HttpdConnData *conn)
{
    int i;
    //  os_printf("Got header %s\n", h);
    if(os_strncmp(h, "GET ", 4)==0 || os_strncmp(h, "POST ", 5)==0) {
        char *e;
        i = 0;                       // Skip past the space after POST/GET
        while(h[i] != ' ') i++;
        conn->url = h + i + 1;
        e = (char*)os_strstr(conn->url, " ");        // Figure out end of url.
        if(e == NULL) return; // wtf?
        *e = 0; // terminate url part
        os_printf("URL = %s\n", conn->url);
        conn->getArgs = (char*)os_strstr(conn->url, "?");   // Parse out the URL part before the GET parameters.
        if(conn->getArgs != 0) {
            *conn->getArgs = 0;
            conn->getArgs++;
            os_printf("GET args = %s\n", conn->getArgs);
        }
        else {
            conn->getArgs=NULL;
        }
    }
    else if(os_strncmp(h, "Content-Length: ", 16) == 0) {
        i = 0;
        while(h[i] != ' ') i++;        // Skip trailing spaces
        conn->postLen=atoi(h+i+1);        // Get POST data length
        // Clamp if too big. Hmm, maybe we should error out instead?
        if(conn->postLen>MAX_POST) conn->postLen=MAX_POST;
        os_printf("Mallocced buffer for %d bytes of post data.\n", conn->postLen);
        conn->postBuff = (char*)os_malloc(conn->postLen+1); // Alloc the memory.
        conn->priv->postPos = 0;
    }
}

// ----------------------------------------------------------------------------
// Callback called when there's data available on a socket.
// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR httpdRecvCb(void *arg, char *data, unsigned short len)
{
    int x;
    char *p, *e;
    char sendBuff[MAX_SENDBUFF_LEN];
    HttpdConnData *conn = httpdFindConnData(arg);
    if(conn == NULL) return;
    conn->priv->sendBuff = sendBuff;
    conn->priv->sendBuffLen = 0;

    for(x = 0; x < len; x++) {
        if(conn->postLen < 0) {              // This byte is a header byte.
            if(conn->priv->headPos != MAX_HEAD_LEN) conn->priv->head[conn->priv->headPos++] = data[x];
            conn->priv->head[conn->priv->headPos]=0;
            if(data[x] == '\n' && (char *)os_strstr(conn->priv->head, "\r\n\r\n") != NULL) {  // Scan for /r/n/r/n
                conn->postLen = 0;         // Indicate we're done with the headers.
                conn->url = NULL;           // Reset url data
                p = conn->priv->head;      // Find end of next header line
                while(p < (&conn->priv->head[conn->priv->headPos-4])) {
                    e = (char *)os_strstr(p, "\r\n");
                    if(e == NULL) break;
                    e[0] = 0;
                    httpdParseHeader(p, conn);
                    p = e + 2;
                }
                if(conn->postLen == 0) {
                    httpdSendResp(conn);       // If we don't need to receive post data, we can send the response now.
                }
            }
        }
        else if(conn->priv->postPos!=-1 && conn->postLen!=0 && conn->priv->postPos <= conn->postLen) {
            conn->postBuff[conn->priv->postPos++] = data[x];  // This byte is a POST byte.
            if(conn->priv->postPos >= conn->postLen) {      // Received post stuff.
                conn->postBuff[conn->priv->postPos] = 0;  //zero-terminate
                conn->priv->postPos = -1;
//              os_printf("Post data: %s\n", conn->postBuff);
                os_printf("%d bytes of post data\n", strlen(conn->postBuff));
                httpdSendResp(conn);  // Send the response.
                break;
            }
        }
    }
    xmitSendBuff(conn);
}

// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR httpdReconCb(void *arg, sint8 err)
{
    HttpdConnData *conn = httpdFindConnData(arg);
    os_printf("ReconCb\n");
    if(conn == NULL) return;  // No idea what to do here. ToDo: figure something out.
}

// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR httpdDisconCb(void *arg)
{
#if 0
    // esp sdk passes through wrong arg here, namely the one of the *listening* socket.
    //If it ever gets fixed, be sure to update the code in this snippet; it's probably out-of-date.
    HttpdConnData *conn=httpdFindConnData(arg);
    os_printf("Disconnected, conn=%p\n", conn);
    if(conn == NULL) return;
    conn->conn = NULL;
    if(conn->cgi != NULL) conn->cgi(conn); // flush cgi data
#endif
    //Just look at all the sockets and kill the slot if needed.
    int i;
    for(i = 0; i < MAX_CONN; i++) {
        if(connData[i].conn != NULL) {
            // Why the >=ESPCONN_CLOSE and not ==? because the stack sometimes de-allocates
            // espconns under our noses, especially when connections are interrupted. The memory
            // is then used for something else, and we can use that to capture *most* of the
            // disconnect cases.
            if(connData[i].conn->state == ESPCONN_NONE || connData[i].conn->state >= ESPCONN_CLOSE) {
                connData[i].conn = NULL;
                if(connData[i].cgi != NULL) connData[i].cgi(&connData[i]); // flush cgi data
                httpdRetireConn(&connData[i]);
            }
        }
    }
}

// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR httpdConnectCb(void *arg) 
{
    struct espconn *conn = arg;
    int i;

    // Find empty conndata in pool
    for(i = 0; i < MAX_CONN; i++) if (connData[i].conn == NULL) break;
    os_printf("Con req, conn=%p, pool slot %d\n", conn, i);
    if(i == MAX_CONN) {
        os_printf("Error: conn pool overflow!\n");
        espconn_disconnect(conn);
        return;
    }
    connData[i].priv =&connPrivData[i];
    connData[i].conn =conn;
    connData[i].priv->headPos = 0;
    connData[i].postBuff = NULL;
    connData[i].priv->postPos = 0;
    connData[i].postLen = -1;

    espconn_regist_recvcb(conn, httpdRecvCb);
    espconn_regist_reconcb(conn, httpdReconCb);
    espconn_regist_disconcb(conn, httpdDisconCb);
    espconn_regist_sentcb(conn, httpdSentCb);
}

// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR httpdInit(HttpdBuiltInUrl *fixedUrls, int port) 
{
    int i;

    for(i = 0; i < MAX_CONN; i++) {
        connData[i].conn = NULL;
    }
    httpdConn.type  = ESPCONN_TCP;
    httpdConn.state = ESPCONN_NONE;
    httpdTcp.local_port = port;
    httpdConn.proto.tcp = &httpdTcp;
    builtInUrls = fixedUrls;

    os_printf("Httpd init, conn=%p\n", &httpdConn);
    espconn_regist_connectcb(&httpdConn, httpdConnectCb);
    espconn_accept(&httpdConn);
}

// ----------------------------------------------------------------------------

