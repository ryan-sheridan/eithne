#include <stddef.h>
#include <stdint.h>

// following guide https://jsandler18.github.io/tutorial/boot.html

// both of these functions take input as a register, which
// is an absolute address. we can write a 4 byte word to the
// register here
static inline void mmio_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t*)reg = data;
}

// here we can read a 4 byte word
static inline uint32_t mmio_read(uint32_t reg) {
    return *(volatile uint32_t*)reg;
}

// delay can be optimised away which is why we inline asm
// really inaccurate way of sleeping, we just loop for a bit
static inline void delay(int32_t count) {
    asm volatile(
        "__delay_%=: subs %[count], %[count], #1; "
        "bne __delay_%=\n"
        : [count] "+r"(count)
        :
        : "cc"
    );
}

// here we have base address for gpio and uart, also each register
// will actually mean something later
enum
{
    // The GPIO registers base address.
    GPIO_BASE = 0x3F200000, // for raspi2 & 3, 0x20200000 for raspi1

    GPPUD = (GPIO_BASE + 0x94),
    GPPUDCLK0 = (GPIO_BASE + 0x98),

    // The base address for UART.
    UART0_BASE = 0x3F201000, // for raspi2 & 3, 0x20201000 for raspi1

    UART0_DR     = (UART0_BASE + 0x00),
    UART0_RSRECR = (UART0_BASE + 0x04),
    UART0_FR     = (UART0_BASE + 0x18),
    UART0_ILPR   = (UART0_BASE + 0x20),
    UART0_IBRD   = (UART0_BASE + 0x24),
    UART0_FBRD   = (UART0_BASE + 0x28),
    UART0_LCRH   = (UART0_BASE + 0x2C),
    UART0_CR     = (UART0_BASE + 0x30),
    UART0_IFLS   = (UART0_BASE + 0x34),
    UART0_IMSC   = (UART0_BASE + 0x38),
    UART0_RIS    = (UART0_BASE + 0x3C),
    UART0_MIS    = (UART0_BASE + 0x40),
    UART0_ICR    = (UART0_BASE + 0x44),
    UART0_DMACR  = (UART0_BASE + 0x48),
    UART0_ITCR   = (UART0_BASE + 0x80),
    UART0_ITIP   = (UART0_BASE + 0x84),
    UART0_ITOP   = (UART0_BASE + 0x88),
    UART0_TDR    = (UART0_BASE + 0x8C),
};

// setting up hardware, consists at setting various configuration flags in registers to get uart actually working
void uart_init()
{
    // disabling all aspects of uart hardware, UART0_CR being the control register
    mmio_write(UART0_CR, 0x00000000);

    // disabling pins 14 and 15 of GPIO
    mmio_write(GPPUD, 0x00000000); // pins should be disabled
    delay(150); // q? why are we sleeping
    mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15)); // what pins are being disabled
    delay(150);

    // whole thing takes affect
    // if we actually want to use GPIO later we will come back here i guess
    mmio_write(GPPUDCLK0, 0x00000000);

    // interrupt clear reg, clear all pending interrupts from UART hardware
    // idk how interrupts work yet
    mmio_write(UART0_ICR, 0x7FF);

    // settings baud rate, 1 and 40 are values calculated based on a forumla thats mentioned in the guide im following
    mmio_write(UART0_IBRD, 1);
    mmio_write(UART0_FBRD, 40);

    // line control, hold data in a 8 item deep FIFO, instead of 1 item deep register,
    // data sent and recieved will have 8 bit long words
    mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

    // disable interrupts from UART by writing 1 to the relevent bits of interrupt mask set clear register
    mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
            (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));

    // write bits 0,8,9 to the control register, bit 0 enables UART hardware,
    // bit 8 enables ability to recieve data, bit 9 enables ability to transmit data
    mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

// reading and writing text
// FR (flags register)
// fr tells us wether the FIFO has any data for us to read and whether
// the rwite FIFO can accept any data
// DR (data register)

// writeup didnt include this data so i will explain when i need to come
// back to understand. we cant just write whenever, we are only allowed to
// write if a specific bit in the flags register is set.
//
// 1 << 5 (shift left 5)
// 00000001 -> 00100000
//
// we are creating a mask over the TXFF (transmit FIFO full)
//
// TXFF = 1, transmit buffer is full, dont write yet
// TXFF = 0, we have space, okay to write
//
// while ( mmio_read(UART0_FR) & (1 << 5) ) { } (while we have space)
void uart_putc(unsigned char c)
{
    while ( mmio_read(UART0_FR) & (1 << 5) ) { }
    mmio_write(UART0_DR, c);
}

// and here, RXFE (bit 4) recieve FIFO empty, so only read when 0
unsigned char uart_getc()
{
    while ( mmio_read(UART0_FR) & (1 << 4) ) { }
    return mmio_read(UART0_DR);
}

// abstraction baby
void uart_puts(const char* str)
{
    for (size_t i = 0; str[i] != '\0'; i ++)
        uart_putc((unsigned char)str[i]);
}

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
    (void) r0;
    (void) r1;
    (void) atags;

    uart_init();
    uart_puts("Hello, kernel World!\r\n");

    while (1) {
        uart_putc(uart_getc());
        uart_putc('\n');
    }
}

