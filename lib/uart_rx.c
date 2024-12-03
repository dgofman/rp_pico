#include "uart_pico.h"
#include "uart_rx.h"
#include <hardware/clocks.h>

#define pio_rx_wrap_target 0
#define pio_rx_wrap 6

static UartRx *uart_rx_instances[NUM_PIO_STATE_MACHINES] = {NULL, NULL, NULL, NULL};

/**
 * @brief PIO RX program instructions.
 */
static const uint16_t pio_rx_program_instructions[] = {
    0xe032, //  0: set    x, 18
    0x2020, //  1: wait   0 pin, 0
    0xa047, //  2: mov    y, osr
    0x0083, //  3: jmp    y--, 3
    0x4001, //  4: in     pins, 1
    0x0042, //  5: jmp    x--, 2
    0x8020, //  6: push   block
};

/**
 * @brief Structure representing the PIO RX program.
 */
static const struct pio_program pio_rx_program = {
    .instructions = pio_rx_program_instructions,
    .length = 7,
    .origin = -1,
};

/**
 * @brief Initializes the PIO RX program with the given parameters.
 *
 * @param pio Pointer to the PIO instance.
 * @param sm The state machine number.
 * @param offset The program offset in PIO memory.
 * @param pin The GPIO pin for RX.
 */
static inline void pio_rx_program_init(PIO pio, uint sm, uint offset, uint pin)
{
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);
    pio_gpio_init(pio, pin);
    gpio_pull_up(pin);
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + pio_rx_wrap_target, offset + pio_rx_wrap);
    sm_config_set_in_pins(&c, pin); // for WAIT, IN
    sm_config_set_jmp_pin(&c, pin); // for JMP
    sm_config_set_in_shift(&c, true, false, 32);
    pio_sm_init(pio, sm, offset, &c);
}

/**
 * @brief Handles the interrupt triggered by the RX FIFO.
 */
void UartRx_handleIRQ()
{
    UartRx *uart = NULL;

    uint sm; /**< State machine number */
    // Loop through the uart_rx_instances array to find the corresponding UartRx instance
    for (sm = 0; sm < NUM_PIO_STATE_MACHINES; sm++)
    {
        uart = uart_rx_instances[sm];
        if (uart && pio_sm_is_rx_fifo_empty(uart->pio, sm) == false)
        {
            break; // We found the correct UartRx instance for the state machine 'sm'
        }
        uart = NULL; // Reset to NULL if not found
    }

    if (!uart)
    {
        return; // If we couldn't find a valid UartRx instance, return early
    }

    UartPico *pico = uart->pico;
    while (!pio_sm_is_rx_fifo_empty(uart->pio, sm))
    {
        uint32_t decode = uart->pio->rxf[sm];
        decode >>= 33 - uart->rxBits;
        uint32_t val = 0;
        for (int b = 0; b < pico->bits + 1; b++)
        {
            val |= (decode & (1 << (b * 2))) ? 1 << b : 0;
        }
        int next_writer = uart->writer + 1;
        if (next_writer == pico->fifoSize)
        {
            next_writer = 0;
        }
        if (next_writer != uart->reader)
        {
            uart->queue[uart->writer] = val & ((1 << pico->bits) - 1);
            asm volatile("" ::: "memory");
            uart->writer = next_writer;
        }
    }
}

/**
 * @brief Initializes the UART receiver with the specified parameters.
 *
 * @param pico Pointer to the UartPico configuration.
 * @param rx Pin number for the RX data.
 * @return Pointer to the initialized UartRx structure or NULL on failure.
 */
UartRx *UartRx_init(UartPico *pico, uint8_t rx)
{
    UartRx *uart = (UartRx *)malloc(sizeof(UartRx));
    if (uart == NULL)
    {
        return NULL;
    }

    uart->pico = pico;
    uart->rx = rx;
    uart->reader = 0;
    uart->writer = 0;

    // Allocate memory for the queue
    uart->queue = (uint8_t *)malloc(pico->fifoSize * 2);
    if (uart->queue == NULL)
    {
        free(uart);
        return NULL;
    }
    return uart;
}

/**
 * @brief Frees the memory allocated for a `UartRx` instance.
 *
 * This function deallocates the memory associated with the `UartRx` structure, ensuring that there are no memory leaks.
 * It should be called when the `UartRx` instance is no longer needed.
 *
 * @param uart Pointer to the `UartRx` instance to be freed.
 */
void UartRx_free(UartRx *uart)
{
    if (uart)
    {
        if (uart->queue)
        {
            free(uart->queue);
        }
        free(uart);
    }
}

/**
 * @brief Activates the UART receiver (sets up the PIO, state machine, and interrupts).
 *
 * @param uart Pointer to the UartRx structure.
 * @return 0 on success, non-zero on failure.
 */
int UartRx_activate(UartRx *uart)
{
    UartPico *pico = uart->pico;
    uart->rxBits = 2 * (pico->bits + pico->stop + 1) - 1;
    int sm; /**< State machine number */
    int offset = UartPico_find_offset_for_program(pico, &uart->pio, &sm, uart->rxBits, &pio_rx_program);
    if (offset < 0)
    {
        return 1; // Return error if the program offset was not found
    }
    gpio_init(uart->rx);             // Initialize the pin
    gpio_set_dir(uart->rx, GPIO_IN); // Set the pin as input
    gpio_pull_up(uart->rx);          // Enable internal pull-up resistor

    pio_rx_program_init(uart->pio, sm, offset, uart->rx);
    pio_sm_clear_fifos(uart->pio, sm); // Remove any existing data

    // Put phase divider into OSR w/o using add'l program memory
    pio_sm_put_blocking(uart->pio, sm, clock_get_hz(clk_sys) / (pico->baud * 2) - 7 /* insns in PIO halfbit loop */);
    pio_sm_exec(uart->pio, sm, pio_encode_pull(false, false));

    // Enable interrupts on rxfifo
    switch (sm)
    {
    case 0:
        pio_set_irq0_source_enabled(uart->pio, pis_sm0_rx_fifo_not_empty, true);
        break;
    case 1:
        pio_set_irq0_source_enabled(uart->pio, pis_sm1_rx_fifo_not_empty, true);
        break;
    case 2:
        pio_set_irq0_source_enabled(uart->pio, pis_sm2_rx_fifo_not_empty, true);
        break;
    case 3:
        pio_set_irq0_source_enabled(uart->pio, pis_sm3_rx_fifo_not_empty, true);
        break;
    }

    uart_rx_instances[sm] = uart; // Register the instance in the static array

    uint8_t irqno = pio_get_index(uart->pio) == 0 ? PIO0_IRQ_0 : PIO1_IRQ_0;
    irq_set_exclusive_handler(irqno, UartRx_handleIRQ);
    irq_set_enabled(irqno, true);

    gpio_set_inover(uart->rx, false);
    pio_sm_set_enabled(uart->pio, sm, true);

    return 0;
}

/**
 * @brief Checks if data is available in the UART RX FIFO.
 *
 * @param uart Pointer to the UartRx structure.
 * @return Number of available bytes in the FIFO.
 */
int UartRx_available(UartRx *uart)
{
    return (uart->writer - uart->reader) % uart->pico->fifoSize;
}

/**
 * @brief Reads data from the UART RX FIFO until a terminator is found.
 *
 * @param uart Pointer to the UartRx structure.
 * @param terminator Character that marks the end of the string.
 * @return The string of data received from the UART.
 */
char *UartRx_readStringUntil(UartRx *uart, char terminator)
{
    UartPico *pico = uart->pico;
    static char result[100]; // Adjust the buffer size if needed
    int idx = 0;
    while (uart->reader != uart->writer)
    {
        char c = (char)uart->queue[uart->reader];
        uart->reader = (uart->reader + 1) % pico->fifoSize; // Move the reader pointer
        if (c == terminator)
        {
            break; // Stop if the terminator is found
        }
        result[idx++] = c;
    }
    result[idx] = '\0'; // Null-terminate the string
    return result;
}
