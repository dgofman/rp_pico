#ifndef UART_RX_H
#define UART_RX_H

#include "uart_pico.h"

#define UART_MAX_BUFFER_LENGTH 256

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Structure representing a UART receiver.
     */
    typedef struct
    {
        UartPico *pico;  /**< Pointer to the associated UartPico configuration */
        PIO pio;         /**< Pointer to the PIO instance */
        int rx;          /**< Pin number for receiving data */
        int rxBits;      /**< Number of bits to read per frame */
        uint8_t *queue;  /**< Pointer to the FIFO buffer */
        uint32_t reader; /**< Reader pointer for FIFO */
        uint32_t writer; /**< Writer pointer for FIFO */
    } UartRx;

    /**
     * @brief Initializes the UartRx structure.
     *
     * @param pico Pointer to the UartPico configuration.
     * @param rx Pin number to use for RX.
     * @return Pointer to the initialized UartRx structure or NULL if failed.
     */
    UartRx *UartRx_init(UartPico *pico, uint8_t rx);
    void UartRx_free(UartRx *uart);
    int UartRx_activate(UartRx *uart);
    int UartRx_available(UartRx *uart);
    char *UartRx_read(UartRx *uart);
    char *UartRx_readLine(UartRx *uart);
    void UartRx_handleIRQ(void);

#ifdef __cplusplus
}
#endif

#endif // UART_RX_H
