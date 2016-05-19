// ----------------------------------------------------------------------------
// code that does the bare minimum to make os_printf work.
// ----------------------------------------------------------------------------
#include <ets_sys.h>
#include <osapi.h>
#include "espmissingincludes.h"
#include "uart_hw.h"

bool nodebugmsgs = 0;

// ----------------------------------------------------------------------------
// Wait until there is room in the FIFO
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR stdoutUartTxd(char c)
{
    while(((READ_PERI_REG(UART_STATUS(0)) >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT) >= 126);
    WRITE_PERI_REG(UART_FIFO(0), c);    // Send the character
}

// ----------------------------------------------------------------------------
// Set a semaphore here to suppress stdout to the UART when comm to the
// can controllers. will use direct reads and writes for that.
// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR stdoutPutchar(char c)
{
    if(nodebugmsgs) return;             // suppress debug messages
    if(c=='\n') stdoutUartTxd('\r');    // convert \n -> \r\n
    stdoutUartTxd(c);
}

// ----------------------------------------------------------------------------
// Initialize the stdio
// ----------------------------------------------------------------------------
void stdoutInit(void)
{
    PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);                   // Enable TxD pin
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
    
    //Set baud rate and other serial parameters to 115200,n,8,1
    uart_div_modify(0, UART_CLK_FREQ/BIT_RATE_115200);
    WRITE_PERI_REG(UART_CONF0(0), (STICK_PARITY_DIS)|(ONE_STOP_BIT << UART_STOP_BIT_NUM_S)| (EIGHT_BITS << UART_BIT_NUM_S));

    //Reset tx & rx fifo
    SET_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST|UART_TXFIFO_RST);
    CLEAR_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST|UART_TXFIFO_RST);
    WRITE_PERI_REG(UART_INT_CLR(0), 0xffff);  // Clear pending interrupts
    os_install_putc1((void *)stdoutPutchar);  // Install our own putchar handler
}

// ----------------------------------------------------------------------------

