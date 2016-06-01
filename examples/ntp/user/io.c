// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Basic IO Routines
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#include <ets_sys.h>
#include <c_types.h>
#include <osapi.h>
#include <user_interface.h>
#include <espconn.h>
#include <gpio.h>
#include "espmissingincludes.h"
#include "uart_hw.h"

// ----------------------------------------------------------------------------
// Global definitions and variables
// ----------------------------------------------------------------------------
#define DEBUG_ON         0        // Hold low to enable debug message output
#define TESTLED          2        // Use GPIO 2 for the TEST LED

// ----------------------------------------------------------------------------
// Global variables
// ----------------------------------------------------------------------------
static ETSTimer resetBtntimer;
bool nodebugmsgs = 0;             // default is have messages

// ----------------------------------------------------------------------------
// Encapsulate gpio_output_set Macro
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR ioLed(int ena)
{
    if(ena) GPIO_OUTPUT_SET(TESTLED, 1);
    else    GPIO_OUTPUT_SET(TESTLED, 0);
}
// ----------------------------------------------------------------------------
// mux the output on and off using gpio pin to sense
// ----------------------------------------------------------------------------
void ICACHE_FLASH_ATTR ioMux(int mux)
{
    if(mux) nodebugmsgs = true;
    else    nodebugmsgs = false;
}

// ----------------------------------------------------------------------------
// Button input IO. If button is pressed, GPIO0 is low, if held pressed
// for 3sec, then released, it will switch the system to dual mode.
// ----------------------------------------------------------------------------
static void ICACHE_FLASH_ATTR resetBtnTimerCb(void *arg)
{
//    ioMux(GPIO_INPUT_GET(DEBUG_ON));      // if jumper is low, allow debug msgs
}

// ----------------------------------------------------------------------------
// kbhit()
// ----------------------------------------------------------------------------
bool kbhit(void)
{
    return(READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S));
}

// ----------------------------------------------------------------------------
// getch()
// ----------------------------------------------------------------------------
char getch(void)
{
    return(READ_PERI_REG(UART_FIFO(UART0)) & 0xFF);
}

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
void ICACHE_FLASH_ATTR stdoutPutchar(char c)
{
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

    os_install_putc1((void *)stdoutPutchar);  // Install our own putchar handler using UART 0
}

// ----------------------------------------------------------------------------
// GPIO Initialization
// ----------------------------------------------------------------------------
void ioInit(void)
{
    stdoutInit();

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);    // Use GPIO2
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);    // Use GPIO0

    gpio_output_set(0,0,  (1 << TESTLED), 0);   // Set TESTLED GPIO to output
    gpio_output_set(0,0,0,(1 << DEBUG_ON)  );   // Set GPIO0 as debug on jumper

    os_timer_disarm(&resetBtntimer);
    os_timer_setfn(&resetBtntimer, resetBtnTimerCb, NULL);
    os_timer_arm(&resetBtntimer, 500, 1);
}

// ----------------------------------------------------------------------------
