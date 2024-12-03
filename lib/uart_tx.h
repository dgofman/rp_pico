#ifndef UART_TX_H
#define UART_TX_H

#include "uart_pico.h"

// Structure to hold the configuration for UART TX
typedef struct
{
    UartPico *pico;       /**< Pointer to the associated UartPico configuration */
    PIO pio;              /**< PIO instance for UART TX */
    int tx;               /**< TX pin number for UART transmission */
    int sm;               /**< State machine number for UART TX in PIO */
} UartTx;

// Function prototypes for initializing, activating, and using the UART TX
UartTx* UartTx_init(UartPico *pico, uint8_t tx);
int UartTx_activate(UartTx *uart);
void UartTx_write(UartTx *uart, uint8_t c);
void UartTx_print(UartTx *uart, const char *str);
void UartTx_println(UartTx *uart, const char *str);
void UartTx_free(UartTx *uart);

#endif // UART_TX_H
