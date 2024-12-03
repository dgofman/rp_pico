#include "uart_pico.h"
#include "uart_tx.h"
#include <hardware/clocks.h>

// Constants for the PIO program to handle UART transmission
#define pio_tx_wrap 5
#define pio_tx_wrap_target 0

/**
 * @brief PIO TX program instructions to handle UART transmission
 */
static const uint16_t pio_tx_program_instructions[] = {
    0xe029, //  0: set    x, 9
    0x98a0, //  1: pull   block           side 1
    0x6001, //  2: out    pins, 1
    0xa046, //  3: mov    y, isr
    0x0084, //  4: jmp    y--, 4
    0x0042, //  5: jmp    x--, 2
};

/**
 * @brief Structure representing the PIO TX program.
 */
static const struct pio_program pio_tx_program = {
    .instructions = pio_tx_program_instructions,
    .length = 6,
    .origin = -1,
};

/**
 * @brief Initializes the PIO (Programmable Input/Output) state machine to handle UART transmission.
 *
 * This function configures the PIO state machine to control the transmission of UART data by setting up
 * the corresponding TX pin and its associated parameters like pin direction, side-set, and wrap settings.
 * It prepares the state machine to output data from the FIFO to the specified TX pin.
 *
 * @param pio PIO instance used to handle the UART transmission.
 * @param sm  State machine number used in PIO.
 * @param offset Offset to the PIO program instructions to be executed.
 * @param pin_tx TX pin used for UART transmission.
 */
static inline void pio_tx_program_init(PIO pio, uint sm, uint offset, uint pin_tx)
{
    // Set the pin to high initially
    pio_sm_set_set_pins(pio, sm, pin_tx, 1);
    // Set pin direction to output
    pio_sm_set_consecutive_pindirs(pio, sm, pin_tx, 1, true);
    pio_gpio_init(pio, pin_tx);                    // Initialize the TX pin in PIO
    pio_sm_config c = pio_get_default_sm_config(); // Get default state machine configuration

    // Set wrap points for the state machine (start and end points)
    sm_config_set_wrap(&c, offset + pio_tx_wrap_target, offset + pio_tx_wrap);

    sm_config_set_sideset(&c, 2, true, false);     // Configure side-set (start/stop bits)
    sm_config_set_out_shift(&c, true, false, 32);  // Shift data out to TX pin
    sm_config_set_out_pins(&c, pin_tx, 1);         // Set which pin to use for output (TX pin)
    sm_config_set_sideset_pins(&c, pin_tx);        // Configure the side-set pins (for timing control)
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX); // Join the state machine’s FIFO to TX

    // Initialize the state machine with the configured settings
    pio_sm_init(pio, sm, offset, &c);
}

/**
 * @brief Initializes a UART transmission instance.
 *
 * This function allocates memory for a `UartTx` structure and initializes it with the provided
 * UART PIO instance and TX pin number. It prepares the structure for subsequent UART transmission operations.
 *
 * @param pico Pointer to the `UartPico` structure that contains PIO settings.
 * @param tx The TX pin to be used for UART transmission.
 *
 * @return A pointer to the newly created `UartTx` structure, or `NULL` if memory allocation fails.
 */
UartTx *UartTx_init(UartPico *pico, uint8_t tx)
{
    // Allocate memory for the UART object
    UartTx *uart = (UartTx *)malloc(sizeof(UartTx));
    if (uart == NULL)
    {                // Check if memory allocation was successful
        return NULL; // Return NULL if allocation failed
    }
    uart->pico = pico; // Store the reference to the UartPico object
    uart->tx = tx;     // Set the TX pin
    return uart;       // Return the initialized UART object
}

/**
 * @brief Frees the memory allocated for a `UartTx` instance.
 *
 * This function deallocates the memory associated with the `UartTx` structure, ensuring that there are no memory leaks.
 * It should be called when the `UartTx` instance is no longer needed.
 *
 * @param uart Pointer to the `UartTx` instance to be freed.
 */
void UartTx_free(UartTx *uart)
{
    if (uart != NULL)
    {               // Check if the pointer is not NULL
        free(uart); // Free the memory occupied by the UART instance
    }
}

/**
 * @brief Activates the UART transmission functionality.
 *
 * This function prepares the PIO state machine for transmitting data over UART. It initializes the TX pin as an output,
 * sets up the PIO program for UART transmission, and starts the state machine. The UART baud rate and other parameters
 * are also configured.
 *
 * @param uart Pointer to the `UartTx` instance.
 *
 * @return 0 if the UART was successfully activated, or 1 if an error occurred during the initialization.
 */
int UartTx_activate(UartTx *uart)
{
    UartPico *pico = uart->pico;
    int txBits = pico->bits + pico->stop + 1; // Calculate total number of bits for the UART frame
    int offset = UartPico_find_offset_for_program(pico, &uart->pio, &uart->sm, txBits, &pio_tx_program);
    if (offset < 0)
    {
        return 1; // Return error if the program offset was not found
    }

    gpio_init(uart->tx);              // Initialize the TX pin
    gpio_set_dir(uart->tx, GPIO_OUT); // Set TX pin direction to output
    gpio_pull_up(uart->tx);           // Enable pull-up resistor for TX pin

    // Initialize the PIO program for UART TX
    pio_tx_program_init(uart->pio, uart->sm, offset, uart->tx);
    pio_sm_clear_fifos(uart->pio, uart->sm); // Clear any existing data in PIO FIFO

    // Set the baud rate and configure the PIO to start transmission
    pio_sm_put_blocking(uart->pio, uart->sm, clock_get_hz(clk_sys) / pico->baud - 2);
    pio_sm_exec(uart->pio, uart->sm, pio_encode_pull(false, false));    // Pull instruction for ISR
    pio_sm_exec(uart->pio, uart->sm, pio_encode_mov(pio_isr, pio_osr)); // Move instruction for ISRs

    gpio_set_outover(uart->tx, false);             // Disable output overdrive for the TX pin
    pio_sm_set_enabled(uart->pio, uart->sm, true); // Enable the PIO state machine

    return 0; // Return success
}

/**
 * @brief Writes a single byte to the UART TX FIFO.
 *
 * This function sends a byte of data to the UART TX FIFO. It checks whether the FIFO is full, and if not, it
 * sends the byte. If the FIFO is full, the function simply returns without sending the byte, allowing other functions
 * to retry later.
 *
 * @param uart Pointer to the `UartTx` instance.
 * @param c The byte to be transmitted over UART.
 */
void UartTx_write(UartTx *uart, uint8_t c)
{
    UartPico *pico = uart->pico;
    uint32_t val = c;

    val |= 7 << pico->bits; // Set the bits according to the specified UART configuration
    val <<= 1;              // Shift the byte to add the start bit (low)

    // Send the byte to the FIFO
    pio_sm_put_blocking(uart->pio, uart->sm, val);
}

/**
 * @brief Prints a null-terminated string via UART transmission.
 *
 * This function sends each character of the provided string to the UART TX FIFO, using the `UartTx_write` function.
 *
 * @param uart Pointer to the `UartTx` instance.
 * @param str The null-terminated string to be transmitted over UART.
 */
void UartTx_print(UartTx *uart, const char *str)
{
    for (size_t i = 0; i < strlen(str); i++)
    {
        UartTx_write(uart, str[i]);
    }
}

/**
 * @brief Prints a string followed by a newline and carriage return.
 *
 * This function sends each character of the string followed by a carriage return and newline character.
 *
 * @param uart Pointer to the `UartTx` instance.
 * @param str The null-terminated string to be transmitted over UART.
 */
void UartTx_println(UartTx *uart, const char *str)
{
    UartTx_print(uart, str);  // Send the string
    UartTx_write(uart, '\r'); // Send carriage return
    UartTx_write(uart, '\n'); // Send newline
}
