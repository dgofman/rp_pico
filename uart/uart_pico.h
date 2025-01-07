#ifndef UART_PICO_H
#define UART_PICO_H

#include <hardware/pio.h>
#include <stdlib.h>
#include <string.h>

#define UART_PICO()      \
    {                    \
        .baud = 9600,    \
        .fifoSize = 128, \
        .stop = 1,       \
        .bits = 8,       \
    }

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Structure representing UART parameters for a specific PIO.
     */
    typedef struct
    {
        unsigned long baud; /**< Baud rate for UART communication */
        size_t fifoSize;    /**< Size of the FIFO buffer */
        uint8_t stop;       /**< Number of stop bits (1 or 2) */
        int bits;           /**< Number of data bits (usually 8) */
        uint32_t used_mask; /**< Unused state machine (SM) for the program */
    } UartPico;

    /**
     * @brief Claims a free state machine (SM) and adds a program to the available instruction memory range for a given GPIO.
     *
     * This function attempts to find an available space in the PIO instruction memory to add a given program.
     * It modifies the program's instructions by encoding the GPIO bit information into the first instruction,
     * and then it attempts to load the modified program into the PIO instruction memory.
     * If successful, the function claims a free state machine and returns the offset at which the program was loaded.
     * If any failure occurs, appropriate error codes are returned.
     *
     * @param pico Pointer to the UartPico instance.
     * @param pio A pointer to the PIO instance where the program will be added. This pointer is updated to reflect
     *            the new state of the PIO instance after the program is added.
     * @param sm A pointer to an integer that will store the claimed state machine number. This state machine will
     *           be used to run the program.
     * @param bits The GPIO bits used in the `pio_encode_set()` instruction. This modifies the program.
     * @param pg A pointer to the PIO program to be loaded into the instruction memory. The program is modified
     *           before being added to the instruction memory.
     *
     * @return The offset where the program was added in the instruction memory, or:
     *         - `-1` if memory allocation for instructions failed.
     *         - `-2` if memory allocation for the program failed.
     *         - `-3` if no space was found in the instruction memory for the program.
     */
    static int UartPico_find_offset_for_program(UartPico *pico, PIO *pio, int *sm, int bits, const pio_program_t *pg)
    {
        PIO bpio = (pio_hw_t *)PIO0_BASE;
        uint32_t program_mask = (1u << pg->length) - 1;                     // Prepare the masks for used instruction space and the program mask
        uint16_t *insn = (uint16_t *)malloc(pg->length * sizeof(uint16_t)); // Allocate memory to store the program instructions
        if (!insn)
        {
            return -1; // If memory allocation for instructions fails.
        }
        memcpy(insn, pg->instructions, pg->length * sizeof(uint16_t)); // Copy the instructions from the original program
        insn[0] = pio_encode_set(pio_x, bits);                         // Modify the first instruction to set GPIO bits using the pio_encode_set function

        // Initialize a new program structure with the modified instructions.
        pio_program_t *program = (pio_program_t *)malloc(sizeof(pio_program_t));
        if (!program)
        {
            free(insn); // Free the allocated memory for instructions if program allocation fails
            return -2;  // If memory allocation for the program fails
        }
        *program = *pg;
        program->instructions = insn; // Set instructions to the newly allocated array

        //  Try to find free space in the instruction memory by checking from the top (higher offsets).
        for (int offset = 32 - program->length; offset >= 0; offset--)
        {
            if (!(pico->used_mask & (program_mask << (uint)offset)))
            {
                for (uint i = 0; i < program->length; ++i)
                {
                    uint16_t instr = program->instructions[i];
                    // If the instruction is a jump, adjust it to the correct offset.
                    bpio->instr_mem[offset + i] = pio_instr_bits_jmp != _pio_major_instr_bits(instr) ? instr : instr + offset;
                }

                // Update the used instruction space mask to reflect the newly occupied space.
                pico->used_mask |= program_mask << offset;
                *pio = bpio;
                *sm = pio_claim_unused_sm(*pio, false); // Claim a free state machine (SM) for the program.

                // Clean up allocated memory and return the offset where the program was added.
                free(insn);
                free(program);
                return offset;
            }
        }

        // If no free space is found, set pio to NULL, free memory
        *pio = NULL;
        free(insn);
        free(program);
        return -3; // To indicate failure (no free space available).
    }

#ifdef __cplusplus
}
#endif

#endif // UART_PICO_H
