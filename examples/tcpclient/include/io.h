// ----------------------------------------------------------------------------

void ICACHE_FLASH_ATTR ioLed(int ena);
void ICACHE_FLASH_ATTR stdoutUartTxd(char c);
void ICACHE_FLASH_ATTR stdoutPutchar(char c);

char getch(void);
bool kbhit(void);
void stdoutInit();
void ioInit(void);


// ----------------------------------------------------------------------------

